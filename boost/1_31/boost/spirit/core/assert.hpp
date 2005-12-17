/*=============================================================================
    Copyright (c) 2001-2003 Joel de Guzman
    Copyright (c) 2002-2003 Hartmut Kaiser
    http://spirit.sourceforge.net/

    Use, modification and distribution is subject to the Boost Software
    License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
    http://www.boost.org/LICENSE_1_0.txt)
=============================================================================*/
#if !defined(BOOST_SPIRIT_ASSERT_HPP)
#define BOOST_SPIRIT_ASSERT_HPP

#include <boost/config.hpp>
#include <boost/throw_exception.hpp>

///////////////////////////////////////////////////////////////////////////////
//
//  BOOST_SPIRIT_ASSERT is used throughout the framework.  It can be
//  overridden by the user. If BOOST_SPIRIT_ASSERT_EXCEPTION is defined,
//  then that will be thrown, otherwise, BOOST_SPIRIT_ASSERT simply turns
//  into a plain assert()
//
///////////////////////////////////////////////////////////////////////////////
#if !defined(BOOST_SPIRIT_ASSERT)
#if defined(NDEBUG)
    #define BOOST_SPIRIT_ASSERT(x)
#elif defined (BOOST_SPIRIT_ASSERT_EXCEPTION)
    #define BOOST_SPIRIT_ASSERT_AUX(f, l, x)                                    \
    do{ if (!(x)) boost::throw_exception(                                       \
        BOOST_SPIRIT_ASSERT_EXCEPTION(f "(" #l "): " #x)); } while(0)
    #define BOOST_SPIRIT_ASSERT(x) SPIRIT_ASSERT_AUX(__FILE__, __LINE__, x)
#else
    #include <cassert>
    #define BOOST_SPIRIT_ASSERT(x) assert(x)
#endif
#endif // !defined(BOOST_SPIRIT_ASSERT)

#endif // BOOST_SPIRIT_ASSERT_HPP