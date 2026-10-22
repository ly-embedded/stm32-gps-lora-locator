#ifndef __COM_DEBUG_H__
#define __COM_DEBUG_H__
#include <string.h>
#include <stdio.h>
/**
 * 程序的环境分为 开发环境 和 生产环境
 * 开发环境: 就是程序员在开发调试代码都是开发环境
 * 生产环境: 程序员已经调试开发完成 烧录到板子中 项目上线 交给用户手里了
 *
 * 开发环境:可以存在debug(printf)
 * 生产环境: 不需要任何的debug (不需要printf)
 *
 * ... 可变参数,表示可以有0个或者多个参数 必须放在形参的最后
 * __VA_ARGS__ 拿到可变参数 并用来传递给其他函数的
 * ## 如果可变参数长度为0 则用来消除前边的逗号
 *
 * __FILE__ 在C语言中是为了拿到当前文件的路径
 * __LINE__ 是拿到当前的行号
 *
 * strchr(str,c) : 从str中从前向后找 找c第一次出现的位置 返回指针
 * strrchr(str,c) : 从str中从后向前找 找c第一次出现的位置 返回指针
 *
 */

// 如果定义了debug 则在开发环境下  如果没有定义debug 则在生产环境下
#define debug // 切换开发和生产环境

#define FILENAME (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define _FILENAME (strrchr(FILENAME, '\\') ? strrchr(FILENAME, '\\') + 1 : FILENAME)

#ifdef debug
#define debug_printf(format, ...) printf("[%s : %d] " format, _FILENAME, __LINE__, ##__VA_ARGS__)
#define debug_printfln(format, ...) printf("[%s : %d] " format "\r\n", _FILENAME, __LINE__, ##__VA_ARGS__)
#else
#define debug_printf(format, ...)
#endif

#endif /* __COM_DEBUG_H__ */
