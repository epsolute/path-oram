#ifndef DEFINITIONS_INCLUDED
#define DEFINITIONS_INCLUDED

#include <boost/config.hpp>
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/tuple/tuple.hpp>
#include <tuple>

template <typename StdTuple, std::size_t... Is>
auto asBoostTuple(StdTuple&& stdTuple, std::index_sequence<Is...>)
{
	return boost::tuple<std::tuple_element_t<Is, std::decay_t<StdTuple>>...>(std::get<Is>(std::forward<StdTuple>(stdTuple))...);
}

template <typename BoostTuple, std::size_t... Is>
auto asStdTuple(BoostTuple&& boostTuple, std::index_sequence<Is...>)
{
	return std::tuple<typename boost::tuples::element<Is, std::decay_t<BoostTuple>>::type...>(boost::get<Is>(std::forward<BoostTuple>(boostTuple))...);
}

template <typename StdTuple>
auto asBoostTuple(StdTuple&& stdTuple)
{
	return asBoostTuple(std::forward<StdTuple>(stdTuple),
						std::make_index_sequence<std::tuple_size<std::decay_t<StdTuple>>::value>());
}

template <typename BoostTuple>
auto asStdTuple(BoostTuple&& boostTuple)
{
	return asStdTuple(std::forward<BoostTuple>(boostTuple),
					  std::make_index_sequence<boost::tuples::length<std::decay_t<BoostTuple>>::value>());
}

template <class... Containers>
auto zip(Containers&... containers) -> decltype(boost::make_iterator_range(
	boost::make_zip_iterator(boost::make_tuple(containers.begin()...)),
	boost::make_zip_iterator(boost::make_tuple(containers.end()...))))
{
	return {boost::make_zip_iterator(boost::make_tuple(containers.begin()...)),
			boost::make_zip_iterator(boost::make_tuple(containers.end()...))};
}

namespace PathORAM
{
	using namespace boost;

	typedef struct
	{
		long from;
		long to;
		long weight;
	} Edge;
}

#endif
