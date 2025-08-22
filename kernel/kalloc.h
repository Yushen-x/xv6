#ifndef __KALLOC_H__
#define __KALLOC_H__

// ==========================================================
// ==   这是关键的修复：让 kalloc.h 包含它自己的依赖   ==
// ==========================================================
#include "types.h"
#include "memlayout.h"
#include "param.h"


// 用于访问物理页引用计数数组
#define PA2PGREF_ID(p) (((uint64)(p) - KERNBASE) / PGSIZE)
#define PGREF_MAX_ENTRIES PA2PGREF_ID(PHYSTOP)

// 通过物理地址获得引用计数
// 这个宏展开后是一个数组元素，是“lvalue”，可以被赋值
#define PA2PGREF(p) pageref[PA2PGREF_ID((uint64)(p))]

// 声明全局变量，让其他文件知道它们的存在
// 定义本身还在 kalloc.c
extern int pageref[PGREF_MAX_ENTRIES];

#endif // __KALLOC_H__
