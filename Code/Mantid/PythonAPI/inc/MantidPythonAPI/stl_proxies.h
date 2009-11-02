#ifndef STL_PROXIES_H_
#define STL_PROXIES_H_

#include <boost/python/class.hpp>
#include <boost/python/errors.hpp>
#include <boost/python/tuple.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>
#include <vector>
#include <set>

namespace Mantid
{
namespace PythonAPI
{

  /// std::vector wrapper
  template <typename ElementType>
  struct vector_proxy
  {
    typedef std::vector<ElementType> w_t;

    static void
    wrap(std::string const& python_name)
    {
      boost::python::class_<w_t, std::auto_ptr<w_t> >(python_name.c_str())
        .def(boost::python::init<w_t const&>())
	.def(boost::python::vector_indexing_suite<w_t>())
      ;
   }

  };
  
  /// std::set wrapper
  // Found this at http://cctbx.svn.sourceforge.net/viewvc/cctbx/trunk/scitbx/stl/set_wrapper.h?view=log
  template <typename ElementType>
  struct set_proxy
  {
    typedef std::set<ElementType> w_t;
    typedef ElementType e_t;

    static void
    insert_element(w_t& self, e_t const& x) { self.insert(x); }

    static void
    insert_set(w_t& self, w_t const& other)
    {
      self.insert(other.begin(), other.end());
    }

    static bool
    contains(w_t const& self, e_t const& x)
    {
      return self.find(x) != self.end();
    }

    static e_t
    getitem(w_t const& self, std::size_t i)
    {
      if (i >= self.size()) 
      {
	PyErr_SetString(PyExc_IndexError, "Index out of range");
	boost::python::throw_error_already_set();
      }
      typename w_t::const_iterator p = self.begin();
      while (i > 0) { p++; i--; }
      return *p;
    }

    static boost::python::tuple
    getinitargs(w_t const& self)
    {
      return boost::python::make_tuple(boost::python::tuple(self));
    }

    static void
    wrap(std::string const& python_name)
    {
      boost::python::class_<w_t, std::auto_ptr<w_t> >(python_name.c_str())
        .def(boost::python::init<w_t const&>())
        .def("size", &w_t::size)
        .def("__len__", &w_t::size)
        .def("insert", insert_element)
        .def("append", insert_element)
        .def("insert", insert_set)
        .def("extend", insert_set)
        .def("erase", (std::size_t(w_t::*)(e_t const&)) &w_t::erase)
        .def("clear", &w_t::clear)
        .def("__contains__", contains)
        .def("__getitem__", getitem)
        .enable_pickling()
        .def("__getinitargs__", getinitargs)
      ;
    }
  };

}
}

#endif //STL_PROXIES_H_
