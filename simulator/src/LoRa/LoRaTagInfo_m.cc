//
// Generated file, do not edit! Created by nedtool 6.0 from LoRa/LoRaTagInfo.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "LoRaTagInfo_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

namespace {
template <class T> inline
typename std::enable_if<std::is_polymorphic<T>::value && std::is_base_of<omnetpp::cObject,T>::value, void *>::type
toVoidPtr(T* t)
{
    return (void *)(static_cast<const omnetpp::cObject *>(t));
}

template <class T> inline
typename std::enable_if<std::is_polymorphic<T>::value && !std::is_base_of<omnetpp::cObject,T>::value, void *>::type
toVoidPtr(T* t)
{
    return (void *)dynamic_cast<const void *>(t);
}

template <class T> inline
typename std::enable_if<!std::is_polymorphic<T>::value, void *>::type
toVoidPtr(T* t)
{
    return (void *)static_cast<const void *>(t);
}

}

namespace flora {

// forward
template<typename T, typename A>
std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec);

// Template rule to generate operator<< for shared_ptr<T>
template<typename T>
inline std::ostream& operator<<(std::ostream& out,const std::shared_ptr<T>& t) { return out << t.get(); }

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
inline typename std::enable_if<!std::is_base_of<omnetpp::cObject, T>::value, std::ostream&>::type
operator<<(std::ostream& out,const T&) {const char *s = omnetpp::opp_typename(typeid(T)); out.put('<'); out.write(s, strlen(s)); out.put('>'); return out;}

// operator<< for std::vector<T>
template<typename T, typename A>
inline std::ostream& operator<<(std::ostream& out, const std::vector<T,A>& vec)
{
    out.put('{');
    for(typename std::vector<T,A>::const_iterator it = vec.begin(); it != vec.end(); ++it)
    {
        if (it != vec.begin()) {
            out.put(','); out.put(' ');
        }
        out << *it;
    }
    out.put('}');

    char buf[32];
    sprintf(buf, " (size=%u)", (unsigned int)vec.size());
    out.write(buf, strlen(buf));
    return out;
}

Register_Class(LoRaTag)

LoRaTag::LoRaTag() : ::inet::TagBase()
{
}

LoRaTag::LoRaTag(const LoRaTag& other) : ::inet::TagBase(other)
{
    copy(other);
}

LoRaTag::~LoRaTag()
{
}

LoRaTag& LoRaTag::operator=(const LoRaTag& other)
{
    if (this == &other) return *this;
    ::inet::TagBase::operator=(other);
    copy(other);
    return *this;
}

void LoRaTag::copy(const LoRaTag& other)
{
    this->centerFrequency = other.centerFrequency;
    this->bandwidth = other.bandwidth;
    this->spreadFactor = other.spreadFactor;
    this->power = other.power;
    this->UseHeader = other.UseHeader;
    this->codeRendundance = other.codeRendundance;
    this->isGeneratedByGateway_ = other.isGeneratedByGateway_;
    this->packetCounter = other.packetCounter;
    this->isHeartbeat_ = other.isHeartbeat_;
}

void LoRaTag::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::TagBase::parsimPack(b);
    doParsimPacking(b,this->centerFrequency);
    doParsimPacking(b,this->bandwidth);
    doParsimPacking(b,this->spreadFactor);
    doParsimPacking(b,this->power);
    doParsimPacking(b,this->UseHeader);
    doParsimPacking(b,this->codeRendundance);
    doParsimPacking(b,this->isGeneratedByGateway_);
    doParsimPacking(b,this->packetCounter);
    doParsimPacking(b,this->isHeartbeat_);
}

void LoRaTag::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::TagBase::parsimUnpack(b);
    doParsimUnpacking(b,this->centerFrequency);
    doParsimUnpacking(b,this->bandwidth);
    doParsimUnpacking(b,this->spreadFactor);
    doParsimUnpacking(b,this->power);
    doParsimUnpacking(b,this->UseHeader);
    doParsimUnpacking(b,this->codeRendundance);
    doParsimUnpacking(b,this->isGeneratedByGateway_);
    doParsimUnpacking(b,this->packetCounter);
    doParsimUnpacking(b,this->isHeartbeat_);
}

inet::Hz LoRaTag::getCenterFrequency() const
{
    return this->centerFrequency;
}

void LoRaTag::setCenterFrequency(inet::Hz centerFrequency)
{
    this->centerFrequency = centerFrequency;
}

inet::Hz LoRaTag::getBandwidth() const
{
    return this->bandwidth;
}

void LoRaTag::setBandwidth(inet::Hz bandwidth)
{
    this->bandwidth = bandwidth;
}

int LoRaTag::getSpreadFactor() const
{
    return this->spreadFactor;
}

void LoRaTag::setSpreadFactor(int spreadFactor)
{
    this->spreadFactor = spreadFactor;
}

inet::W LoRaTag::getPower() const
{
    return this->power;
}

void LoRaTag::setPower(inet::W power)
{
    this->power = power;
}

bool LoRaTag::getUseHeader() const
{
    return this->UseHeader;
}

void LoRaTag::setUseHeader(bool UseHeader)
{
    this->UseHeader = UseHeader;
}

int LoRaTag::getCodeRendundance() const
{
    return this->codeRendundance;
}

void LoRaTag::setCodeRendundance(int codeRendundance)
{
    this->codeRendundance = codeRendundance;
}

bool LoRaTag::isGeneratedByGateway() const
{
    return this->isGeneratedByGateway_;
}

void LoRaTag::setIsGeneratedByGateway(bool isGeneratedByGateway)
{
    this->isGeneratedByGateway_ = isGeneratedByGateway;
}

int LoRaTag::getPacketCounter() const
{
    return this->packetCounter;
}

void LoRaTag::setPacketCounter(int packetCounter)
{
    this->packetCounter = packetCounter;
}

bool LoRaTag::isHeartbeat() const
{
    return this->isHeartbeat_;
}

void LoRaTag::setIsHeartbeat(bool isHeartbeat)
{
    this->isHeartbeat_ = isHeartbeat;
}

class LoRaTagDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_centerFrequency,
        FIELD_bandwidth,
        FIELD_spreadFactor,
        FIELD_power,
        FIELD_UseHeader,
        FIELD_codeRendundance,
        FIELD_isGeneratedByGateway,
        FIELD_packetCounter,
        FIELD_isHeartbeat,
    };
  public:
    LoRaTagDescriptor();
    virtual ~LoRaTagDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyname) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyname) const override;
    virtual int getFieldArraySize(void *object, int field) const override;
    virtual void setFieldArraySize(void *object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(void *object, int field, int i) const override;
    virtual std::string getFieldValueAsString(void *object, int field, int i) const override;
    virtual void setFieldValueAsString(void *object, int field, int i, const char *value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual void *getFieldStructValuePointer(void *object, int field, int i) const override;
    virtual void setFieldStructValuePointer(void *object, int field, int i, void *ptr) const override;
};

Register_ClassDescriptor(LoRaTagDescriptor)

LoRaTagDescriptor::LoRaTagDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(flora::LoRaTag)), "inet::TagBase")
{
    propertynames = nullptr;
}

LoRaTagDescriptor::~LoRaTagDescriptor()
{
    delete[] propertynames;
}

bool LoRaTagDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<LoRaTag *>(obj)!=nullptr;
}

const char **LoRaTagDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *LoRaTagDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int LoRaTagDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 9+basedesc->getFieldCount() : 9;
}

unsigned int LoRaTagDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_centerFrequency
        FD_ISEDITABLE,    // FIELD_bandwidth
        FD_ISEDITABLE,    // FIELD_spreadFactor
        FD_ISEDITABLE,    // FIELD_power
        FD_ISEDITABLE,    // FIELD_UseHeader
        FD_ISEDITABLE,    // FIELD_codeRendundance
        FD_ISEDITABLE,    // FIELD_isGeneratedByGateway
        FD_ISEDITABLE,    // FIELD_packetCounter
        FD_ISEDITABLE,    // FIELD_isHeartbeat
    };
    return (field >= 0 && field < 9) ? fieldTypeFlags[field] : 0;
}

const char *LoRaTagDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "centerFrequency",
        "bandwidth",
        "spreadFactor",
        "power",
        "UseHeader",
        "codeRendundance",
        "isGeneratedByGateway",
        "packetCounter",
        "isHeartbeat",
    };
    return (field >= 0 && field < 9) ? fieldNames[field] : nullptr;
}

int LoRaTagDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 'c' && strcmp(fieldName, "centerFrequency") == 0) return base+0;
    if (fieldName[0] == 'b' && strcmp(fieldName, "bandwidth") == 0) return base+1;
    if (fieldName[0] == 's' && strcmp(fieldName, "spreadFactor") == 0) return base+2;
    if (fieldName[0] == 'p' && strcmp(fieldName, "power") == 0) return base+3;
    if (fieldName[0] == 'U' && strcmp(fieldName, "UseHeader") == 0) return base+4;
    if (fieldName[0] == 'c' && strcmp(fieldName, "codeRendundance") == 0) return base+5;
    if (fieldName[0] == 'i' && strcmp(fieldName, "isGeneratedByGateway") == 0) return base+6;
    if (fieldName[0] == 'p' && strcmp(fieldName, "packetCounter") == 0) return base+7;
    if (fieldName[0] == 'i' && strcmp(fieldName, "isHeartbeat") == 0) return base+8;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *LoRaTagDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "inet::Hz",    // FIELD_centerFrequency
        "inet::Hz",    // FIELD_bandwidth
        "int",    // FIELD_spreadFactor
        "inet::W",    // FIELD_power
        "bool",    // FIELD_UseHeader
        "int",    // FIELD_codeRendundance
        "bool",    // FIELD_isGeneratedByGateway
        "int",    // FIELD_packetCounter
        "bool",    // FIELD_isHeartbeat
    };
    return (field >= 0 && field < 9) ? fieldTypeStrings[field] : nullptr;
}

const char **LoRaTagDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *LoRaTagDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int LoRaTagDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    LoRaTag *pp = (LoRaTag *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

void LoRaTagDescriptor::setFieldArraySize(void *object, int field, int size) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldArraySize(object, field, size);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaTag *pp = (LoRaTag *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'LoRaTag'", field);
    }
}

const char *LoRaTagDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaTag *pp = (LoRaTag *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string LoRaTagDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaTag *pp = (LoRaTag *)object; (void)pp;
    switch (field) {
        case FIELD_centerFrequency: return unit2string(pp->getCenterFrequency());
        case FIELD_bandwidth: return unit2string(pp->getBandwidth());
        case FIELD_spreadFactor: return long2string(pp->getSpreadFactor());
        case FIELD_power: return unit2string(pp->getPower());
        case FIELD_UseHeader: return bool2string(pp->getUseHeader());
        case FIELD_codeRendundance: return long2string(pp->getCodeRendundance());
        case FIELD_isGeneratedByGateway: return bool2string(pp->isGeneratedByGateway());
        case FIELD_packetCounter: return long2string(pp->getPacketCounter());
        case FIELD_isHeartbeat: return bool2string(pp->isHeartbeat());
        default: return "";
    }
}

void LoRaTagDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaTag *pp = (LoRaTag *)object; (void)pp;
    switch (field) {
        case FIELD_centerFrequency: pp->setCenterFrequency(Hz(string2double(value))); break;
        case FIELD_bandwidth: pp->setBandwidth(Hz(string2double(value))); break;
        case FIELD_spreadFactor: pp->setSpreadFactor(string2long(value)); break;
        case FIELD_power: pp->setPower(W(string2double(value))); break;
        case FIELD_UseHeader: pp->setUseHeader(string2bool(value)); break;
        case FIELD_codeRendundance: pp->setCodeRendundance(string2long(value)); break;
        case FIELD_isGeneratedByGateway: pp->setIsGeneratedByGateway(string2bool(value)); break;
        case FIELD_packetCounter: pp->setPacketCounter(string2long(value)); break;
        case FIELD_isHeartbeat: pp->setIsHeartbeat(string2bool(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaTag'", field);
    }
}

const char *LoRaTagDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

void *LoRaTagDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    LoRaTag *pp = (LoRaTag *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

void LoRaTagDescriptor::setFieldStructValuePointer(void *object, int field, int i, void *ptr) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaTag *pp = (LoRaTag *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaTag'", field);
    }
}

} // namespace flora

