
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_TYPETRAITS_HPP
#define INCLUDE_RCF_TYPETRAITS_HPP

#include <boost/type_traits.hpp>

namespace RCF {

    template<typename T>
    struct IsFundamental : public boost::is_fundamental<T>
    {};

#if defined(_MSC_VER) && _MSC_VER < 1310

    template<>
    struct IsFundamental<long double> : boost::mpl::true_
    {};

    template<>
    struct IsFundamental<__int64> : boost::mpl::true_
    {};

    template<>
    struct IsFundamental<unsigned __int64> : boost::mpl::true_
    {};

#endif

    template<typename T>
    struct IsConst : public boost::is_const<T>
    {};

    template<typename T>
    struct IsPointer : public boost::is_pointer<T>
    {};

    template<typename T>
    struct IsReference : public boost::is_reference<T>
    {};

    template<typename T>
    struct RemovePointer : public boost::remove_pointer<T>
    {};

    template<typename T>
    struct RemoveReference : public boost::remove_reference<T>
    {};

    template<typename T>
    struct RemoveCv : public boost::remove_cv<T>
    {};

    namespace IDL {

        template<typename T> struct InParameter;
        template<typename T> class InParameter_Value;
        template<typename T> class InParameter_Ref;
        template<typename T> class InParameter_Ptr;

        template<typename T> struct OutParameter;
        template<typename T> class OutParameter_Value;
        template<typename T> class OutParameter_Ref;
        template<typename T> class OutParameter_CRef;
        template<typename T> class OutParameter_Ptr;

    } // namespace IDL

} // namespace RCF

namespace SF {

    template<typename T> struct GetIndirection;

} // namespace SF

#if defined(_MSC_VER) && _MSC_VER <= 1310

#define RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(T)                           \
namespace RCF {                                                                     \
                                                                                    \
    template<>                                                                      \
    struct IsReference<T > : public boost::mpl::false_                              \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct IsReference<T &> : public boost::mpl::true_                              \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct IsReference<const T &> : public boost::mpl::true_                        \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemoveReference<T &> : public boost::mpl::identity<T >                   \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemoveReference<const T &> : public boost::mpl::identity<const T >       \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct IsPointer<T > : public boost::mpl::false_                                \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct IsPointer<T *> : public boost::mpl::true_                                \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct IsPointer<T const *> : public boost::mpl::true_                          \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct IsPointer<T * const> : public boost::mpl::true_                          \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemovePointer<T > : public boost::mpl::identity<T >                      \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemovePointer<T *> : public boost::mpl::identity<T >                     \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemovePointer<T * const> : public boost::mpl::identity<T >               \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemovePointer<T const *> : public boost::mpl::identity<const T >         \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemoveCv<T > : public boost::mpl::identity<T >                           \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemoveCv<const T > : public boost::mpl::identity<T >                     \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemoveCv<volatile T > : public boost::mpl::identity<T >                  \
    {};                                                                             \
                                                                                    \
    template<>                                                                      \
    struct RemoveCv<const volatile T > : public boost::mpl::identity<T >            \
    {};                                                                             \
                                                                                    \
    namespace IDL {                                                                                         \
                                                                                                            \
        template<>                                                                                          \
        struct OutParameter<T > : public boost::mpl::identity< OutParameter_Value<T > >                     \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct OutParameter<T *> : public boost::mpl::identity< OutParameter_Ptr<T *> >                     \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct OutParameter<T &> : public boost::mpl::identity< OutParameter_Ref<T &> >                     \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct OutParameter<const T &> : public boost::mpl::identity< OutParameter_CRef<const T &> >        \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct InParameter<T > : public boost::mpl::identity< InParameter_Value<T > >                       \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct InParameter<T *> : public boost::mpl::identity< InParameter_Ptr<T *> >                       \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct InParameter<const T *> : public boost::mpl::identity< InParameter_Ptr<const T *> >           \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct InParameter<T &> : public boost::mpl::identity< InParameter_Ref<T &> >                       \
        {};                                                                                                 \
                                                                                                            \
        template<>                                                                                          \
        struct InParameter<const T &> : public boost::mpl::identity< InParameter_Ref<const T &> >           \
        {};                                                                                                 \
                                                                                                            \
    }                                                                                                       \
                                                                                                            \
}                                                                                                           \
                                                                                    \
namespace SF {                                                                      \
                                                                                    \
    template<>                                                                      \
    struct GetIndirection<T >                                                       \
    {                                                                               \
        typedef boost::mpl::int_<0> Level;                                          \
        typedef T Base;                                                             \
    };                                                                              \
                                                                                    \
    template<>                                                                      \
    struct GetIndirection<T *>                                                      \
    {                                                                               \
        typedef boost::mpl::int_<1> Level;                                          \
        typedef T Base;                                                             \
    };                                                                              \
                                                                                    \
    template<>                                                                      \
    struct GetIndirection<T const *>                                                \
    {                                                                               \
        typedef boost::mpl::int_<1> Level;                                          \
        typedef T Base;                                                             \
    };                                                                              \
                                                                                    \
    template<>                                                                      \
    struct GetIndirection<T * const *>                                              \
    {                                                                               \
        typedef boost::mpl::int_<2> Level;                                          \
        typedef T Base;                                                             \
    };                                                                              \
                                                                                    \
    template<>                                                                      \
    struct GetIndirection<T const * *>                                              \
    {                                                                               \
        typedef boost::mpl::int_<2> Level;                                          \
        typedef T Base;                                                             \
    };                                                                              \
                                                                                    \
    template<>                                                                      \
    struct GetIndirection<T const * const *>                                        \
    {                                                                               \
        typedef boost::mpl::int_<2> Level;                                          \
        typedef T Base;                                                             \
    };                                                                              \
                                                                                    \
}

#else

#define RCF_BROKEN_COMPILER_TYPE_TRAITS_SPECIALIZATION(T)

#endif

#endif // ! INCLUDE_RCF_TYPETRAITS_HPP
