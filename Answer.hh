#ifndef INCLUDE_ANSWER_HH
#define INCLUDE_ANSWER_HH

#include <boost/optional/optional_fwd.hpp>
#include <boost/property_tree/ptree_fwd.hpp>


enum Answer {
	DONT_CARE,
	NON_NULL_TERMINATED,
	NULL_TERMINATED
};


struct AnswerTranslator {
	typedef std::string internal_type;
	typedef Answer external_type;
	boost::optional<external_type> get_value(const internal_type &);
	boost::optional<internal_type> put_value(const external_type &);
};


namespace boost {
	namespace property_tree {
		template<> struct translator_between<AnswerTranslator::internal_type, AnswerTranslator::external_type> {
			typedef AnswerTranslator type;
		};
	}
}


#endif // !INCLUDE_ANSWER_HH
