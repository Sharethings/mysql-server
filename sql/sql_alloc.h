#ifndef SQL_ALLOC_INCLUDED
#define SQL_ALLOC_INCLUDED
/* Copyright (c) 2012, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   51 Franklin Street, Suite 500, Boston, MA 02110-1335 USA */

#include "my_sys.h"         // TRASH
#include "thr_malloc.h"     // alloc_root

/**
  MySQL standard memory allocator class. You have to inherit the class
  in order to use it.
*/
// flyyear mysql 标准的内存分配类，想要分配内存都必须继承这个类
// 这个类啥成员变量都没有，纯粹是重载了new和delete操作
class Sql_alloc
{
public:
    // flyyear 这面再次查看c++的相关知识（好久不看都忘了) 得出来的结论为：
    // 1. 类的继承，static的方法是可以被继承的，但是是隐藏的，所以如果在子类里面再次定义，会被覆盖
    // 2. operator new 这种重载操作符，默认是static的
    // 3.
    // 不是所有的基类的析构函数必须定义为virtual的（好长时间搞错了），如果没有多态的需求，是不需要定义为virtual的，因为虽然这样也没什么坏处，但是占用的空间是大幅的增加，原来可能只需要默认的一个字节的，现在需要4B（32bit机器），因为需要一个指针vptr指向vtal虚表
    // 4. 继承中构造函数和析构函数调用的顺序，不说了，这个没问题
    // 5. 对于构造函数和析构函数定义为inline也没有问题,也是为了和普通的方法一样的inline效果
    // 综上，这面没有问题，如果Sql_alloc的方法没有重载operator new，会调用这面的方法
    // 这就是学习别人代码的作用

    // flyyear
    // 当我们将上述（标准库中new/delete的各重载版本）运算符函数定义成类的成员时，它们是隐式静态的。我们无须显示地声明
    // static，当然这么做也不会引发错误。
  static void *operator new(size_t size) throw ()
  {
    return sql_alloc(size);
  }
  static void *operator new[](size_t size) throw ()
  {
    return sql_alloc(size);
  }
  static void *operator new[](size_t size, MEM_ROOT *mem_root) throw ()
  { return alloc_root(mem_root, size); }
  static void *operator new(size_t size, MEM_ROOT *mem_root) throw ()
  { return alloc_root(mem_root, size); }
  static void operator delete(void *ptr, size_t size) { TRASH(ptr, size); }
  static void operator delete(void *ptr, MEM_ROOT *mem_root)
  { /* never called */ }
  static void operator delete[](void *ptr, MEM_ROOT *mem_root)
  { /* never called */ }
  static void operator delete[](void *ptr, size_t size) { TRASH(ptr, size); }

  // flyyear 这面的构造函数和析构函数定义成内联的不会有问题? 
  // 这面也是为了快速编译，直接将源码放在调用的地方
  // 虚函数才必须把析构函数设置为虚的 其他的不用
  inline Sql_alloc() {}
  inline ~Sql_alloc() {}
};

#endif // SQL_ALLOC_INCLUDED
