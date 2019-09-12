//==========================================================================
//
//        jffs2_test.c
//
//        jffs2 performance test for BP2016
//
//==========================================================================
// Author(s):
// Contributors:  BSP
// Date:          2017-10-18
// Description:   jffs2 performance test for bp2016
//===========================================================================
//                                INCLUDES
//===========================================================================
#include <cyg/kernel/kapi.h>
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <cyg/hal/a7/cortex_a7.h>
#include <cyg/infra/diag.h>

#ifdef CYGPKG_FS_JFFS2
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <cyg/fileio/fileio.h>
#include <cyg/io/flash.h>
#include <cyg/fileio/dirent.h>

#include <cyg/infra/testcase.h>

#include <pkgconf/fs_jffs2.h>	// Address of JFFS2

#define JFFS2_TEST_DEV "/dev/flash/0/0x800000,0x400000" 

#define SHOW_RESULT( _fn, _res ) \
    printf("FAIL: " #_fn "() returned %ld %s\n", \
           (unsigned long)_res, _res<0?strerror(errno):"");

#define jffs2_printf(fmt, args...)                                                               \
    printf("[jffs2 test] %s:%d " fmt "\n", __func__, __LINE__, ##args);

static void listdir( char *name, int statp, int numexpected, int *numgot );
void checkcwd( const char *cwd );

char buf[1024];
char buf1[1024];

static void dump_buffer(void *buffer, cyg_uint32 len)
{
    cyg_uint32 i = 0;
    cyg_uint8 *buf = (cyg_uint8 *) buffer;

    printf("\n");
    for (i = 0; i < len; i++) {
		if(!(i%16))
			printf("\n0x%x:", i);
        printf("%02x ", *(buf + i));
    }
    printf("\n");
}

int do_jffs2_testing(shell_cmd_t *cptr, const unsigned int argc, const char **argv)
{
    int err;
    FILE *stream;
    long pos;
    int i, j;
    char test_file[25] = "/test_file";
    int existingdirents=-1;

    if (strcmp(argv[1], "mount") == 0)
    {
        CYG_TEST_INFO("mount /");    
        err = mount( JFFS2_TEST_DEV, "/", "jffs2" );

        if( err < 0 ) SHOW_RESULT( mount, err );    

        err = chdir( "/" );
        if( err < 0 ) SHOW_RESULT( chdir, err );

        checkcwd( "/" );
    }

    if (strcmp(argv[1], "read") == 0 && (argc > 2))
    {
        jffs2_printf("Read file name as argv[2]=%s", argv[2]);

        jffs2_printf("############ read %s ################", argv[2]);
        stream = fopen(argv[2],"r+");
        //stream = fopen(argv[2],"r+");
        if (!stream) {
            diag_printf("FAIL: fopen() returned NULL, %s\n", strerror(errno));
            CYG_TEST_FINISH("done");
        }
        err = fseek(stream, 0, SEEK_END);
        pos = ftell(stream);
        err = fseek(stream, 0, SEEK_SET);

        jffs2_printf("read file length %d", pos);
        j = sizeof(buf1);
        for(i=0; i<pos; )
        {
            if(pos >= j) 
            {
                err = fread(buf1,j,1, stream);
                dump_buffer(buf1, j);
                pos -= j;
            }else{
                err = fread(buf1,pos,1, stream);
                dump_buffer(buf1, pos);
                pos -= pos;
            }
            if (err != 1) SHOW_RESULT( fread, err );
        }

        err = fclose(stream);
        if (err != 0) SHOW_RESULT( fclose, err );
    }

    if (strcmp(argv[1], "test") == 0)
    {
        if(argc > 2)
        {
            jffs2_printf("Write file name as argv[2]=%s, len %d", argv[2], strlen(argv[2]));
            memset(test_file, 0, strlen(test_file));
            memcpy(test_file, argv[2], strlen(argv[2]));
        }

        jffs2_printf("Creating FILE: %s, size %d ", test_file, sizeof(buf));    

        stream = fopen(test_file,"w+");
        if (!stream) {
            diag_printf("FAIL: fopen() returned NULL, %s\n", strerror(errno));
            CYG_TEST_FINISH("done");
        }

        /* Write a buffer full of cyclic numbers */
        for (i = 0; i < sizeof(buf); i++) {
            buf[i] = i % 256;
        }

        CYG_TEST_INFO("writing test pattern");    
        err=fwrite(buf,sizeof(buf), 1, stream);
        if ( err < 0 ) SHOW_RESULT( fwrite, err );
        jffs2_printf("buf size %d ,   res = %d", sizeof(buf), err);
        CYG_TEST_INFO("writing test pattern done");    

        /* The current position should be the same size as the buffer */
        pos = ftell(stream);

        diag_printf("pos = %d", pos);

        if (pos < 0) SHOW_RESULT( ftell, pos );
        if (pos != sizeof(buf))
            diag_printf("<FAIL>: ftell is not telling the truth.");

        CYG_TEST_INFO("fseek()ing to beginning and writing");    

        /* Seek back to the beginning of the file */
        err = fseek(stream, 0, SEEK_SET);
        if ( err < 0 ) SHOW_RESULT( fseek, err );

        pos = ftell(stream);

        if (pos < 0) SHOW_RESULT( ftell, pos );
        if (pos != 0) CYG_TEST_FAIL("ftell is not telling the truth");

        /* Write 4 zeros to the beginning of the file */
        for (i = 0; i < 4; i++) {
            buf[i] = 0;
        }

        err = fwrite(buf, 4, 1, stream);
        if ( err < 0 ) SHOW_RESULT( fwrite, err );

        /* Check the pointer is at 4 */
        pos = ftell(stream);

        if (pos < 0) SHOW_RESULT( ftell, pos );
        if (pos != 4)  CYG_TEST_FAIL("ftell is not telling the truth");

        CYG_TEST_INFO("closing file");

        /* Close the file, open it up again and read it back */
        err = fclose(stream);
        if (err != 0) SHOW_RESULT( fclose, err );

        jffs2_printf("open file %s", test_file);
        stream = fopen(test_file, "r+");
        if (!stream) {
            diag_printf("<FAIL>: fopen() returned NULL, %s\n", strerror(errno));
        }

        err = fread(buf1,sizeof(buf1),1, stream);
        if (err != 1) SHOW_RESULT( fread, err );

        CYG_TEST_INFO("Comparing contents");
        if (memcmp(buf, buf1, sizeof(buf1))) {
            CYG_TEST_FAIL("File contents inconsistent");
        }

        CYG_TEST_INFO("closing file");

        err = fclose(stream);
        if (err != 0) SHOW_RESULT( fclose, err );

        jffs2_printf("open file %s", test_file);
        stream = fopen(test_file, "r+");
        if (!stream) {
            diag_printf("<FAIL>: fopen() returned NULL, %s\n", strerror(errno));
        }

        CYG_TEST_INFO("fseek()ing past the end to create a hole");
        /* Seek 1K after the end of the file */
        err = fseek(stream, sizeof(buf), SEEK_END);
        if ( err < 0 ) SHOW_RESULT( fseek, err );

        pos = ftell(stream);

        if (pos < 0) SHOW_RESULT( ftell, pos );
        if (pos != (2*sizeof(buf))) CYG_TEST_FAIL("ftell is not telling the truth");

        CYG_TEST_INFO("writing test pattern");    
        jffs2_printf("write size %d", sizeof(buf));
        err=fwrite(buf,sizeof(buf), 1, stream);
        if ( err < 0 ) SHOW_RESULT( fwrite, err );

        pos = ftell(stream);

        if (pos < 0) SHOW_RESULT( ftell, pos );
        if (pos != (3*sizeof(buf))) CYG_TEST_FAIL("ftell is not telling the truth");

        CYG_TEST_INFO("closing file");
        err = fclose(stream);
        if (err != 0) SHOW_RESULT( fclose, err );

        jffs2_printf("open file %s", test_file);
        stream = fopen(test_file, "r+");
        if (!stream) {
            diag_printf("<FAIL>: fopen() returned NULL, %s\n", strerror(errno));
        }

        err = fread(buf1,sizeof(buf1),1, stream);
        if (err != 1) SHOW_RESULT( fread, err );

        CYG_TEST_INFO("Comparing contents");
        if (memcmp(buf, buf1, sizeof(buf1))) {
            CYG_TEST_FAIL("File contents inconsistent");
        }

        err = fread(buf1,sizeof(buf1),1, stream);
        if (err != 1) SHOW_RESULT( fread, err );

        for (i = 0; i< sizeof(buf); i++) {
            if (buf1[i] != 0)
                CYG_TEST_FAIL("Hole does not contain zeros");
        }

        err = fread(buf1,sizeof(buf1),1, stream);
        if (err != 1) SHOW_RESULT( fread, err );

        if (memcmp(buf, buf1, sizeof(buf1))) {
            CYG_TEST_FAIL("File contents inconsistent");
        }

        CYG_TEST_INFO("closing file");

        /* Close the file */
        err = fclose(stream);
        if (err != 0) SHOW_RESULT( fclose, err );
    }

    if (strcmp(argv[1], "ls") == 0) {
        if(argc > 2)
        {
            jffs2_printf("ls argv[2]=%s, len %d ", argv[2], strlen(argv[2]));

            checkcwd(argv[2]);
            listdir(argv[2], true, -1, &existingdirents );
        }else{
            listdir( ".", true, -1, &existingdirents );
        }
    }

    if (strcmp(argv[1], "cd") == 0) {
        if(argc > 2)
        {
            jffs2_printf("chdir to argv[2]=%s, len %d", argv[2], strlen(argv[2]));

            err = chdir(argv[2]);
            if( err < 0 ) SHOW_RESULT( chdir, err );
        }else{
            jffs2_printf("chdir to /");
            err = chdir( "/" );
            if( err < 0 ) SHOW_RESULT( chdir, err );
        }
    }

    if ((strcmp(argv[1], "rm") == 0) && (argc > 2)) {

        jffs2_printf("rm %s", argv[2]);

        err = rmdir(argv[2]);
        if( err < 0 ){
            err = unlink(argv[2]);    
            if( err < 0 ) SHOW_RESULT( rm, err );
        }
    }

    if ((strcmp(argv[1], "mkdir") == 0) && (argc > 2)) {
        jffs2_printf("mkdir %s", argv[2]);

        err = mkdir(argv[2], 0);
        if( err < 0 ) SHOW_RESULT( mkdir, err );
    }

    if (strcmp(argv[1], "umount") == 0)
    {
        CYG_TEST_INFO("umount /");
        err = umount( "/" );
        if( err < 0 ) SHOW_RESULT( umount, err );    
    }
}

//==========================================================================

static void listdir( char *name, int statp, int numexpected, int *numgot )
{
    int err;
    DIR *dirp;
    int num=0;
    
    diag_printf("<INFO>: reading directory %s\n",name);
    
    dirp = opendir( name );
    if( dirp == NULL ) SHOW_RESULT( opendir, -1 );

    for(;;)
    {
        struct dirent *entry = readdir( dirp );
        
        if( entry == NULL )
            break;
        num++;
        diag_printf("<INFO>: entry %14s",entry->d_name);
#ifdef CYGPKG_FS_FAT_RET_DIRENT_DTYPE
        diag_printf(" d_type %2x", entry->d_type);
#endif
        if( statp )
        {
            char fullname[PATH_MAX];
            struct stat sbuf;

            if( name[0] )
            {
                strcpy(fullname, name );
                if( !(name[0] == '/' && name[1] == 0 ) )
                    strcat(fullname, "/" );
            }
            else fullname[0] = 0;
            
            strcat(fullname, entry->d_name );
            
            err = stat( fullname, &sbuf );
            if( err < 0 )
            {
                if( errno == ENOSYS )
                    diag_printf(" <no status available>");
                else SHOW_RESULT( stat, err );
            }
            else
            {
                diag_printf(" [mode %08x ino %08x nlink %d size %ld]",
                            sbuf.st_mode,sbuf.st_ino,sbuf.st_nlink,(long)sbuf.st_size);
            }
#ifdef CYGPKG_FS_FAT_RET_DIRENT_DTYPE
            if ((entry->d_type & S_IFMT) != (sbuf.st_mode & S_IFMT))
              CYG_TEST_FAIL("File mode's don't match between dirent and stat");
#endif
        }

        diag_printf("\n");
    }

    err = closedir( dirp );
    if( err < 0 ) SHOW_RESULT( stat, err );
    if (numexpected >= 0 && num != numexpected)
        CYG_TEST_FAIL("Wrong number of dir entries\n");
    if ( numgot != NULL )
        *numgot = num;
}

//==========================================================================

//==========================================================================

void checkcwd( const char *cwd )
{
    static char cwdbuf[PATH_MAX];
    char *ret;

    ret = getcwd( cwdbuf, sizeof(cwdbuf));
    if( ret == NULL ) SHOW_RESULT( getcwd, (int)ret );    

    if( strcmp( cwdbuf, cwd ) != 0 )
    {
        diag_printf( "cwdbuf %s cwd %s\n",cwdbuf, cwd );
        CYG_TEST_FAIL( "Current directory mismatch");
    }
}

//==========================================================================
#endif
