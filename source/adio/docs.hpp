#ifndef ADIO_DOCS_HPP_INCLUDED
#define ADIO_DOCS_HPP_INCLUDED

// Macros used for documentation purposes only
#define ADIO_DOC_IMPLDEF(...) __VA_ARGS__
#define ADIO_DOC_IMPLDEF2(...) __VA_ARGS__
#define ADIO_DOC_IMPLDEF3(...) __VA_ARGS__
#define ADIO_DOC_IMPLDEF4(...) __VA_ARGS__
#define ADIO_DOC_UNSPEC(...) __VA_ARGS__
#define ADIO_DOC_UNSPEC2(...) __VA_ARGS__
#define ADIO_DOC_UNSPEC3(...) __VA_ARGS__
#define ADIO_DOC_UNSPEC4(...) __VA_ARGS__

#define ADIO_IGNORE(...)
#define ADIO_JUST(...) __VA_ARGS__

#ifndef DOXYGEN_GENERATING_DOCUMENTATION
#define ADIO_DOCS_LIE(documented, ...) ADIO_JUST
#endif

/**
 * @mainpage Hello!
 *
 * This is the main page
 */

/**
 * @page custom_adaptors Adapting types to adio::value and adio::row
 *
 * In addition to database handling, Adio provides facilities for helping to
 * serialize custom datatypes to and from the basic ``adio::value`` and
 * ``adio::row`` classes. The simplistic types representable in databases may be
 * entirely insufficient to encode the invariants that we wish to implement in
 * custom classes and data structures. Ensuring constraints via the database at
 * runtime is costly compared to simply letting the C++ type system handle it at
 * compile-time.
 *
 * Say we have a type named ``Widget`` that we wish to use as a column in a
 * database. Let us also assume that ``Widget`` can be serialized into a
 * lossless textual representation using some ``to_string()`` function, and
 * recreated using ``Widget::from_string()``. This means that we can use a
 * SQL ``TEXT`` field to store ``Widget`` objects.
 *
 * We can tell ``adio::value`` how to work with ``Widget`` by specializing the
 * ``adio::value_adaptor`` class template.
 *
 * ~~~cpp
 *  namespace adio
 *  {
 *
 *  template<> struct value_adaptor<Widget>
 *  {
 *      using base_type = std::string;
 *      enum { nullable = false };
 *      static Widget convert(const base_type& str)
 *      {
 *          return Widget::from_string(str);
 *      }
 *      static base_type convert(const Widget& widget)
 *      {
 *          return to_string(Widget);
 *      }
 *  };
 *
 *  }
 * ~~~
 *
 * And that's it! Some explaination may be required:
 *
 * - ``base_type`` must be a typedef to the type which we serialize *to* and
 * deserialize *from*. It must be either one of the basic types supported by
 * ``adio::value``, or a type which also has a valid specialization of
 * ``value_adaptor``.
 *
 * - ``nullable`` must be a constant expression, either ``true`` or ``false``.
 * We'll talk about this a bit more below.
 *
 * - Two overloads of ``convert``: One must accept an instance of ``base_type``
 * and return our custom type; this is the *deserialization* function. The other
 * should accept an instance of our custom type and return a new instance of
 * ``base_type``; this is the *serialization* function.
 *
 * If we have a custom type that has a "NULL" state, or something akin to it,
 * (ie. ``boost::optional<T>``), we use the ``nullable`` member to tell Adio
 * about it. Adaptors with ``nullable`` set to ``true`` have a few more
 * requirements. For example, here is a value adaptor for ``boost::optional``:
 *
 * ~~~cpp
 *  namespace adio
 *  {
 *
 *  template<typename Data> struct value_adaptor<boost::optional<Data>>
 *  {
 *      using base_type = Data;
 *      enum { nullable = true }; // Set this to true
 *      static base_type convert(const boost::optional<Data>& opt)
 *      {
 *          return *opt; // Note: No null check here
 *      }
 *      static boost::optional<Data> convert(const base_type& base)
 *      {
 *          return boost::optional<Data>{base};
 *      }
 *      static bool is_null(const boost::optional<Data>& opt)
 *      {
 *          return bool{ opt };
 *      }
 *      static boost::optional<Data> null() { return boost::none; }
 *      {
 *          return boost::none;
 *      }
 *  };
 *
 *  }
 * ~~~
 *
 * There are two more static member functions required for a ``nullable``
 * adaptor:
 *
 * - ``is_null`` accepts an instance of our adapted type and should return
 * ``true`` *iff* the object represents the NULL state. In this case,
 * ``boost::optional`` is "null" if it is not engaged.
 *
 * - ``null`` accepts no arguments and should return an instance of our adapted
 * type that represents the NULL state. For ``boost::optional``, we simple
 * construct an optional from ``boost::none``.
 *
 * It is guaranteed that Adio will never invoke ``convert(T) -> base_type`` with
 * a "null" instance of T. Thus in the example above, we need not check that the
 * ``optional`` instance is a null option, and we simply retrieve the value from
 * inside.
 */

#endif // ADIO_DOCS_HPP_INCLUDED
