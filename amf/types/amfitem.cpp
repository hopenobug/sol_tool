#include "amfitem.hpp"
#include "types/amfarray.hpp"
#include "types/amfbool.hpp"
#include "types/amfdate.hpp"
#include "types/amfdouble.hpp"
#include "types/amfinteger.hpp"
#include "types/amfstring.hpp"
#include "utils/amfitemptr.hpp"

#define DEF_VALUE_FUNC(marker, type_name)                                                                                        \
    decltype(Amf##type_name::value) &AmfItem::as##type_name() {                                                                  \
        if (valueType == marker) {                                                                                               \
            auto &v = dynamic_cast<Amf##type_name &>(*this);                                                                     \
            return v.value;                                                                                                      \
        }                                                                                                                        \
        throw std::exception("type is not " ###type_name);                                                                       \
    }

namespace amf {
AmfItem &AmfItem::operator[](int index) {
    if (valueType == AMF_ARRAY) {
        auto &arr = dynamic_cast<AmfArray &>(*this);
        if (index < 0) {
            index += arr.dense.size();
        }
        return *arr.dense[index];
    }
    throw std::exception("type is not Array");
}

AmfItem &AmfItem::operator[](const std::string &key) {
    if (valueType == AMF_ARRAY) {
        auto &arr = dynamic_cast<AmfArray &>(*this);
        return *arr.associative[key];
    }
    throw std::exception("type is not Array");
}


DEF_VALUE_FUNC(AMF_STRING, String)
DEF_VALUE_FUNC(AMF_INTEGER, Integer)
DEF_VALUE_FUNC(AMF_DOUBLE, Double)
DEF_VALUE_FUNC(AMF_TRUE, Bool)
DEF_VALUE_FUNC(AMF_DATE, Date)

} // namespace amf