#ifndef __DVT_COMMON_H_
#define	__DVT_COMMON_H_

#define DVT_CASE_SHORT_NAME_LENGTH    (16)

struct DVT_CNXT_ST {
    int notused;
};

typedef struct DVT_CNXT_ST DVT_CNXT_T;

typedef unsigned char* 	(*DVT_CASE_GET_DESCRIPTION) (void);
typedef unsigned int 	(*DVT_CASE_AUOTO_FN)(DVT_CNXT_T *pCnxt, unsigned int argc, const char **);
typedef int 		(*DVT_CASE_ON_ENTER_FN)(void);
typedef void 		(*DVT_CASE_ON_EXIT_FN) (void);
typedef int (*DVT_CASE_REG_MENU_FN)(DYN_SHELL_ITEM_T * p_shell);

typedef struct DVT_CASE_ST {  
    unsigned int         		case_id;  //ID of the case
    unsigned char    			name[DVT_CASE_SHORT_NAME_LENGTH];
    unsigned int			progress; //and the result. 
    DVT_CASE_GET_DESCRIPTION     	cmd_description;
    DVT_CASE_GET_DESCRIPTION     	cmd_long_description;
    DVT_CASE_GET_DESCRIPTION     	menu_description;
    DVT_CASE_AUOTO_FN  			auto_test;
    DVT_CASE_ON_ENTER_FN 		on_enter;
    DVT_CASE_ON_EXIT_FN 		on_exit;
    DVT_CASE_REG_MENU_FN 		reg_menu;
//    const DVT_CASE_IOMUX_SETTING_T*   	iomux;
}DVT_CASE_T;

typedef struct DVT_CASE_INFO_ST
{
   DVT_CASE_T* 		p_case_descriptor;
   unsigned int  	result;
} DVT_CASE_INFO_T;

typedef DVT_CASE_T * (* DVT_GET_CASE_DESCRIPTOR_T)(DVT_CNXT_T *pCnxt);
extern DVT_GET_CASE_DESCRIPTOR_T __get_case_descriptor_handle_begin[];
extern DVT_GET_CASE_DESCRIPTOR_T __get_case_descriptor_handle_end[];

#define DECLARE_DVT_CASE(get_case_descriptor_fn)  \
static DVT_GET_CASE_DESCRIPTOR_T __descriptor_func_##get_case_descriptor_fn\
                                  __attribute__((used)) \
                            __attribute__((__section__("func.get_case_descriptor"))) = get_case_descriptor_fn

#define DVT_SHELL_ADD_CMD(p_shell,  short_name, rep,help_info, long_desc, cmd_func)        \
        dyn_shell_add_command(p_shell, (unsigned char*)short_name, rep, (unsigned char*)help_info, (unsigned char*)long_desc, cmd_func)

#define DVT_SHELL_ADD_SUB_SHELL(p_shell,  short_name,help_info, shell_func)        \
        dyn_shell_add_sub_shell(p_shell, (unsigned char*) short_name, (unsigned char*)help_info, shell_func)


#define DVT_CASE_RESULT_NOTTEST (0xFFFFFFFF)
#define DVT_CASE_RESULT_FAIL   (0xFFFF0000)
#define DVT_CASE_RESULT_NA     (0xFFFE0000)      //Not applicable.
#define DVT_CASE_RESULT_BLOCK  (0xFFFD0000)      //condition is not ready..
#define DVT_CASE_RESULT_SUCCESS (0)
#define DVT_CASE_RESULT_PASS   DVT_CASE_RESULT_SUCCESS

#endif //__DVT_COMMON_H_
