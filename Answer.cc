#include "Answer.hh"

#include <boost/bimap.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/optional.hpp>

using namespace boost;


typedef bimap<AnswerTranslator::external_type, AnswerTranslator::internal_type> AnswerNames;
static const AnswerNames answerNames = assign::list_of<AnswerNames::relation>
	(DONT_CARE, "don't care")
	(NON_NULL_TERMINATED, "not NULL terminated")
	(NULL_TERMINATED, "NULL terminated");


template <typename Map>
static optional<typename remove_const<typename Map::data_type>::type> findMaybe(const Map &map, const typename Map::key_type &key) {
	typedef optional<typename remove_const<typename Map::data_type>::type> Result;
	const auto found = map.find(key);
	return found == map.end() ? Result() : Result(found->second);
}


optional<AnswerTranslator::external_type> AnswerTranslator::get_value(const AnswerTranslator::internal_type &text) {
	return findMaybe(answerNames.right, text);
}


optional<AnswerTranslator::internal_type> AnswerTranslator::put_value(const AnswerTranslator::external_type &value) {
	return findMaybe(answerNames.left, value);
}
