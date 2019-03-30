/**********************************************************************
 Freeciv - Copyright (C) 1996 - A Kjeldberg, L Gregersen, P Unold
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2, or (at your option)
 any later version.
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
***********************************************************************/

#ifndef FC__LISTENER_H
#define FC__LISTENER_H

#include <set>

/***************************************************************************
  Helper template to connect C and C++ code.

  This class is a helper to create Java-like "listeners" for "events"
  generated in C code. If the C interface defines a callback foo() and
  you want to use it as an event, you first declare a C++ interface:

  ~~~~~{.cpp}
  class foo_listener : public listener<foo_listener>
  {
  public:
    virtual void foo() = 0;
  };
  ~~~~~

  The listener needs some static data. Declaring it is as simple as putting
  a macro in some source file:

  ~~~~~{.cpp}
  FC_CPP_DECLARE_LISTENER(foo_listener)
  ~~~~~

  Then, you call the listeners from the implementation of the C interface:

  ~~~~~{.cpp}
  void foo()
  {
    foo_listener::invoke(&foo_listener::foo);
  }
  ~~~~~

  This will invoke foo() on all foo_listener objects.

  == Listening to events

  Listening to events is done by inheriting from a listener. When your
  object is ready to receive events, it should call the listener's @c listen
  function. This will typically be done in the constructor:

  ~~~~~{.cpp}
  class bar : private foo_listener
  {
  public:
    explicit bar();
    void foo();
  };

  bar::bar()
  {
    // Initialize the object here
    foo_listener::listen();
  }
  ~~~~~

  == Passing arguments

  Passing arguments to the listeners is very simple: you write them after
  the function pointer. For instance, let's say foo() takes an int. It can
  be passed to the listeners as follows:

  ~~~~~{.cpp}
  void foo(int argument)
  {
    foo_listener::invoke(&foo_listener::foo, argument);
  }
  ~~~~~

  As there may be an arbitrary number of listeners, passing mutable data
  through invoke() is discouraged.

  == Technical details

  This class achieves its purpose using the Curiously Recurring Template
  Pattern, hence the weird parent for listeners:

  ~~~~~{.cpp}
  class foo_listener : public listener<foo_listener>
  ~~~~~

  The template argument is used to specialize object storage and member
  function invocation. Compilers should be able to inline calls to invoke(),
  leaving only the overhead of looping on all instances.

  @warning Implementation is not thread-safe.
***************************************************************************/
template<class _type_>
class listener
{
public:
  // The type a given specialization supports.
  typedef _type_ type_t;

private:
  // All instances of type_t that have called listen().
  static std::set<type_t *> instances;

protected:
  explicit listener();
  virtual ~listener();

  void listen();

public:
  template<class _member_fct_>
  static void invoke(_member_fct_ function);

  template<class _member_fct_, class _arg1_t_>
  static void invoke(_member_fct_ function, _arg1_t_ arg);

  template<class _member_fct_, class _arg1_t_, class _arg2_t_>
  static void invoke(_member_fct_ function, _arg1_t_ arg1, _arg2_t_ arg2);
};

/***************************************************************************
  Macro to declare the static data needed by listener<> classes
***************************************************************************/
#define FC_CPP_DECLARE_LISTENER(_type_) \
  template<> \
  std::set<_type_ *> listener<_type_>::instances = std::set<_type_ *>();

/***************************************************************************
  Constructor
***************************************************************************/
template<class _type_>
listener<_type_>::listener()
{}

/***************************************************************************
  Starts listening to events
***************************************************************************/
template<class _type_>
void listener<_type_>::listen()
{
  // If you get an error here, your listener likely doesn't inherit from the
  // listener<> correctly. See the class documentation.
  instances.insert(static_cast<type_t *>(this));
}

/***************************************************************************
  Destructor
***************************************************************************/
template<class _type_>
listener<_type_>::~listener()
{
  instances.erase(reinterpret_cast<type_t *>(this));
}

/***************************************************************************
  Invokes a member function on all instances of an listener type. Template
  parameters are meant to be automatically deduced.

  Zero-parameter overload.

  @param function The member function to call
***************************************************************************/
template<class _type_>
template<class _member_fct_>
void listener<_type_>::invoke(_member_fct_ function)
{
  typename std::set<type_t *>::iterator it = instances.begin();
  typename std::set<type_t *>::iterator end = instances.end();
  for ( ; it != end; ++it) {
    ((*it)->*function)();
  }
}

/***************************************************************************
  Invokes a member function on all instances of an listener type. Template
  parameters are meant to be automatically deduced.

  One-parameter overload.

  @param function The member function to call
  @param arg      The argument to call the function with
***************************************************************************/
template<class _type_>
template<class _member_fct_, class _arg1_t_>
void listener<_type_>::invoke(_member_fct_ function, _arg1_t_ arg)
{
  typename std::set<type_t *>::iterator it = instances.begin();
  typename std::set<type_t *>::iterator end = instances.end();
  for ( ; it != end; ++it) {
    ((*it)->*function)(arg);
  }
}

/***************************************************************************
  Invokes a member function on all instances of an listener type. Template
  parameters are meant to be automatically deduced.

  Two-parameters overload.

  @param function The member function to call
  @param arg1     The first argument to pass to the function
  @param arg2     The second argument to pass to the function
***************************************************************************/
template<class _type_>
template<class _member_fct_, class _arg1_t_, class _arg2_t_>
void listener<_type_>::invoke(_member_fct_ function,
                              _arg1_t_ arg1, _arg2_t_ arg2)
{
  typename std::set<type_t *>::iterator it = instances.begin();
  typename std::set<type_t *>::iterator end = instances.end();
  for ( ; it != end; ++it) {
    ((*it)->*function)(arg1, arg2);
  }
}

#endif // FC__LISTENER_H
