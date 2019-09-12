#ifndef __DVT_CMD_H_
#define __DVT_CMD_H_
unsigned long simple_strtoul(const char *cp, char **endp,
				unsigned int base);
void register_common_cmd(DYN_SHELL_ITEM_T* p_common_shell);

#endif  /* __DVT_CMD_H_ */
