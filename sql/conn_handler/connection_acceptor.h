/*
   Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifndef CONNECTION_ACCEPTOR_INCLUDED
#define CONNECTION_ACCEPTOR_INCLUDED

#include "channel_info.h"               // Channel_info
#include "connection_handler_manager.h" // Connection_handler_manager

/**
  This class presents a generic interface to initialize and run
  a connection event loop for different types of listeners and
  a callback functor to call on the connection event from the
  listener that listens for connection. Listener type should
  be a class providing methods setup_listener, listen_for_
  connection_event and close_listener. The Connection event
  callback functor object would on receiving connection event
  from the client to process the connection.
*/
// flyyear 定义一个类模版
// 类模版允许用户为类定义一种模式，使得类中的某些数据成员、默写成员函数的参数、某些成员函数的返回值，能够取任意类型（包括系统预定义的和用户自定义的)
// 根据下面的模版的参数调用的函数情况，使用这个类模版的类需要完成三个函数
// setup_listener()、listen_for_connection_event()、close_listener()
template <typename Listener> class Connection_acceptor
{
  Listener *m_listener;

public:
  Connection_acceptor(Listener *listener)
  : m_listener(listener)
  { }

  ~Connection_acceptor()
  {
    delete m_listener;
  }

  /**
    Initialize a connection acceptor.

    @retval   return true if initialization failed, else false.
  */
  bool init_connection_acceptor()
  {
    return m_listener->setup_listener();
  }

  /**
    Connection acceptor loop to accept connections from clients.
  */
  // flyyear 这面一个循环来接受客户端的连接
  void connection_event_loop()
  {
    Connection_handler_manager *mgr= Connection_handler_manager::get_instance();
    // flyyear 这面的abort_loop是一个全局变量 且是volatile
    while (!abort_loop)
    {
      Channel_info *channel_info= m_listener->listen_for_connection_event();
      if (channel_info != NULL)
        mgr->process_new_connection(channel_info);
    }
  }

  /**
    Close the listener.
  */
  void close_listener()
  {
    m_listener->close_listener();
  }

};
#endif // CONNECTION_ACCEPTOR_INCLUDED
