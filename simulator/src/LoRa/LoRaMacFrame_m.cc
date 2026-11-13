//
// Generated file, do not edit! Created by nedtool 6.0 from LoRa/LoRaMacFrame.msg.
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
#include "LoRaMacFrame_m.h"

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

Register_Class(LoRaMacFrame)

LoRaMacFrame::LoRaMacFrame() : ::inet::FieldsChunk()
{
}

LoRaMacFrame::LoRaMacFrame(const LoRaMacFrame& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

LoRaMacFrame::~LoRaMacFrame()
{
}

LoRaMacFrame& LoRaMacFrame::operator=(const LoRaMacFrame& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void LoRaMacFrame::copy(const LoRaMacFrame& other)
{
    this->transmitterAddress = other.transmitterAddress;
    this->receiverAddress = other.receiverAddress;
    this->sequenceNumber = other.sequenceNumber;
    this->LoRaTP = other.LoRaTP;
    this->LoRaCF = other.LoRaCF;
    this->LoRaSF = other.LoRaSF;
    this->LoRaBW = other.LoRaBW;
    this->LoRaCR = other.LoRaCR;
    this->LoRaUseHeader = other.LoRaUseHeader;
    this->RSSI = other.RSSI;
    this->SNIR = other.SNIR;
    this->miRSSI = other.miRSSI;
    this->miSNIR = other.miSNIR;
    this->isGeneratedByGateway_ = other.isGeneratedByGateway_;
}

void LoRaMacFrame::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->transmitterAddress);
    doParsimPacking(b,this->receiverAddress);
    doParsimPacking(b,this->sequenceNumber);
    doParsimPacking(b,this->LoRaTP);
    doParsimPacking(b,this->LoRaCF);
    doParsimPacking(b,this->LoRaSF);
    doParsimPacking(b,this->LoRaBW);
    doParsimPacking(b,this->LoRaCR);
    doParsimPacking(b,this->LoRaUseHeader);
    doParsimPacking(b,this->RSSI);
    doParsimPacking(b,this->SNIR);
    doParsimPacking(b,this->miRSSI);
    doParsimPacking(b,this->miSNIR);
    doParsimPacking(b,this->isGeneratedByGateway_);
}

void LoRaMacFrame::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->transmitterAddress);
    doParsimUnpacking(b,this->receiverAddress);
    doParsimUnpacking(b,this->sequenceNumber);
    doParsimUnpacking(b,this->LoRaTP);
    doParsimUnpacking(b,this->LoRaCF);
    doParsimUnpacking(b,this->LoRaSF);
    doParsimUnpacking(b,this->LoRaBW);
    doParsimUnpacking(b,this->LoRaCR);
    doParsimUnpacking(b,this->LoRaUseHeader);
    doParsimUnpacking(b,this->RSSI);
    doParsimUnpacking(b,this->SNIR);
    doParsimUnpacking(b,this->miRSSI);
    doParsimUnpacking(b,this->miSNIR);
    doParsimUnpacking(b,this->isGeneratedByGateway_);
}

const inet::MacAddress& LoRaMacFrame::getTransmitterAddress() const
{
    return this->transmitterAddress;
}

void LoRaMacFrame::setTransmitterAddress(const inet::MacAddress& transmitterAddress)
{
    handleChange();
    this->transmitterAddress = transmitterAddress;
}

const inet::MacAddress& LoRaMacFrame::getReceiverAddress() const
{
    return this->receiverAddress;
}

void LoRaMacFrame::setReceiverAddress(const inet::MacAddress& receiverAddress)
{
    handleChange();
    this->receiverAddress = receiverAddress;
}

int LoRaMacFrame::getSequenceNumber() const
{
    return this->sequenceNumber;
}

void LoRaMacFrame::setSequenceNumber(int sequenceNumber)
{
    handleChange();
    this->sequenceNumber = sequenceNumber;
}

double LoRaMacFrame::getLoRaTP() const
{
    return this->LoRaTP;
}

void LoRaMacFrame::setLoRaTP(double LoRaTP)
{
    handleChange();
    this->LoRaTP = LoRaTP;
}

inet::Hz LoRaMacFrame::getLoRaCF() const
{
    return this->LoRaCF;
}

void LoRaMacFrame::setLoRaCF(inet::Hz LoRaCF)
{
    handleChange();
    this->LoRaCF = LoRaCF;
}

int LoRaMacFrame::getLoRaSF() const
{
    return this->LoRaSF;
}

void LoRaMacFrame::setLoRaSF(int LoRaSF)
{
    handleChange();
    this->LoRaSF = LoRaSF;
}

inet::Hz LoRaMacFrame::getLoRaBW() const
{
    return this->LoRaBW;
}

void LoRaMacFrame::setLoRaBW(inet::Hz LoRaBW)
{
    handleChange();
    this->LoRaBW = LoRaBW;
}

int LoRaMacFrame::getLoRaCR() const
{
    return this->LoRaCR;
}

void LoRaMacFrame::setLoRaCR(int LoRaCR)
{
    handleChange();
    this->LoRaCR = LoRaCR;
}

bool LoRaMacFrame::getLoRaUseHeader() const
{
    return this->LoRaUseHeader;
}

void LoRaMacFrame::setLoRaUseHeader(bool LoRaUseHeader)
{
    handleChange();
    this->LoRaUseHeader = LoRaUseHeader;
}

double LoRaMacFrame::getRSSI() const
{
    return this->RSSI;
}

void LoRaMacFrame::setRSSI(double RSSI)
{
    handleChange();
    this->RSSI = RSSI;
}

double LoRaMacFrame::getSNIR() const
{
    return this->SNIR;
}

void LoRaMacFrame::setSNIR(double SNIR)
{
    handleChange();
    this->SNIR = SNIR;
}

double LoRaMacFrame::getMiRSSI() const
{
    return this->miRSSI;
}

void LoRaMacFrame::setMiRSSI(double miRSSI)
{
    handleChange();
    this->miRSSI = miRSSI;
}

double LoRaMacFrame::getMiSNIR() const
{
    return this->miSNIR;
}

void LoRaMacFrame::setMiSNIR(double miSNIR)
{
    handleChange();
    this->miSNIR = miSNIR;
}

bool LoRaMacFrame::isGeneratedByGateway() const
{
    return this->isGeneratedByGateway_;
}

void LoRaMacFrame::setIsGeneratedByGateway(bool isGeneratedByGateway)
{
    handleChange();
    this->isGeneratedByGateway_ = isGeneratedByGateway;
}

class LoRaMacFrameDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_transmitterAddress,
        FIELD_receiverAddress,
        FIELD_sequenceNumber,
        FIELD_LoRaTP,
        FIELD_LoRaCF,
        FIELD_LoRaSF,
        FIELD_LoRaBW,
        FIELD_LoRaCR,
        FIELD_LoRaUseHeader,
        FIELD_RSSI,
        FIELD_SNIR,
        FIELD_miRSSI,
        FIELD_miSNIR,
        FIELD_isGeneratedByGateway,
    };
  public:
    LoRaMacFrameDescriptor();
    virtual ~LoRaMacFrameDescriptor();

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

Register_ClassDescriptor(LoRaMacFrameDescriptor)

LoRaMacFrameDescriptor::LoRaMacFrameDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(flora::LoRaMacFrame)), "inet::FieldsChunk")
{
    propertynames = nullptr;
}

LoRaMacFrameDescriptor::~LoRaMacFrameDescriptor()
{
    delete[] propertynames;
}

bool LoRaMacFrameDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<LoRaMacFrame *>(obj)!=nullptr;
}

const char **LoRaMacFrameDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *LoRaMacFrameDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int LoRaMacFrameDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 14+basedesc->getFieldCount() : 14;
}

unsigned int LoRaMacFrameDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        0,    // FIELD_transmitterAddress
        0,    // FIELD_receiverAddress
        FD_ISEDITABLE,    // FIELD_sequenceNumber
        FD_ISEDITABLE,    // FIELD_LoRaTP
        FD_ISEDITABLE,    // FIELD_LoRaCF
        FD_ISEDITABLE,    // FIELD_LoRaSF
        FD_ISEDITABLE,    // FIELD_LoRaBW
        FD_ISEDITABLE,    // FIELD_LoRaCR
        FD_ISEDITABLE,    // FIELD_LoRaUseHeader
        FD_ISEDITABLE,    // FIELD_RSSI
        FD_ISEDITABLE,    // FIELD_SNIR
        FD_ISEDITABLE,    // FIELD_miRSSI
        FD_ISEDITABLE,    // FIELD_miSNIR
        FD_ISEDITABLE,    // FIELD_isGeneratedByGateway
    };
    return (field >= 0 && field < 14) ? fieldTypeFlags[field] : 0;
}

const char *LoRaMacFrameDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "transmitterAddress",
        "receiverAddress",
        "sequenceNumber",
        "LoRaTP",
        "LoRaCF",
        "LoRaSF",
        "LoRaBW",
        "LoRaCR",
        "LoRaUseHeader",
        "RSSI",
        "SNIR",
        "miRSSI",
        "miSNIR",
        "isGeneratedByGateway",
    };
    return (field >= 0 && field < 14) ? fieldNames[field] : nullptr;
}

int LoRaMacFrameDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 't' && strcmp(fieldName, "transmitterAddress") == 0) return base+0;
    if (fieldName[0] == 'r' && strcmp(fieldName, "receiverAddress") == 0) return base+1;
    if (fieldName[0] == 's' && strcmp(fieldName, "sequenceNumber") == 0) return base+2;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaTP") == 0) return base+3;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaCF") == 0) return base+4;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaSF") == 0) return base+5;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaBW") == 0) return base+6;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaCR") == 0) return base+7;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaUseHeader") == 0) return base+8;
    if (fieldName[0] == 'R' && strcmp(fieldName, "RSSI") == 0) return base+9;
    if (fieldName[0] == 'S' && strcmp(fieldName, "SNIR") == 0) return base+10;
    if (fieldName[0] == 'm' && strcmp(fieldName, "miRSSI") == 0) return base+11;
    if (fieldName[0] == 'm' && strcmp(fieldName, "miSNIR") == 0) return base+12;
    if (fieldName[0] == 'i' && strcmp(fieldName, "isGeneratedByGateway") == 0) return base+13;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *LoRaMacFrameDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "inet::MacAddress",    // FIELD_transmitterAddress
        "inet::MacAddress",    // FIELD_receiverAddress
        "int",    // FIELD_sequenceNumber
        "double",    // FIELD_LoRaTP
        "inet::Hz",    // FIELD_LoRaCF
        "int",    // FIELD_LoRaSF
        "inet::Hz",    // FIELD_LoRaBW
        "int",    // FIELD_LoRaCR
        "bool",    // FIELD_LoRaUseHeader
        "double",    // FIELD_RSSI
        "double",    // FIELD_SNIR
        "double",    // FIELD_miRSSI
        "double",    // FIELD_miSNIR
        "bool",    // FIELD_isGeneratedByGateway
    };
    return (field >= 0 && field < 14) ? fieldTypeStrings[field] : nullptr;
}

const char **LoRaMacFrameDescriptor::getFieldPropertyNames(int field) const
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

const char *LoRaMacFrameDescriptor::getFieldProperty(int field, const char *propertyname) const
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

int LoRaMacFrameDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    LoRaMacFrame *pp = (LoRaMacFrame *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

void LoRaMacFrameDescriptor::setFieldArraySize(void *object, int field, int size) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldArraySize(object, field, size);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaMacFrame *pp = (LoRaMacFrame *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'LoRaMacFrame'", field);
    }
}

const char *LoRaMacFrameDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaMacFrame *pp = (LoRaMacFrame *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string LoRaMacFrameDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaMacFrame *pp = (LoRaMacFrame *)object; (void)pp;
    switch (field) {
        case FIELD_transmitterAddress: return pp->getTransmitterAddress().str();
        case FIELD_receiverAddress: return pp->getReceiverAddress().str();
        case FIELD_sequenceNumber: return long2string(pp->getSequenceNumber());
        case FIELD_LoRaTP: return double2string(pp->getLoRaTP());
        case FIELD_LoRaCF: return unit2string(pp->getLoRaCF());
        case FIELD_LoRaSF: return long2string(pp->getLoRaSF());
        case FIELD_LoRaBW: return unit2string(pp->getLoRaBW());
        case FIELD_LoRaCR: return long2string(pp->getLoRaCR());
        case FIELD_LoRaUseHeader: return bool2string(pp->getLoRaUseHeader());
        case FIELD_RSSI: return double2string(pp->getRSSI());
        case FIELD_SNIR: return double2string(pp->getSNIR());
        case FIELD_miRSSI: return double2string(pp->getMiRSSI());
        case FIELD_miSNIR: return double2string(pp->getMiSNIR());
        case FIELD_isGeneratedByGateway: return bool2string(pp->isGeneratedByGateway());
        default: return "";
    }
}

void LoRaMacFrameDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaMacFrame *pp = (LoRaMacFrame *)object; (void)pp;
    switch (field) {
        case FIELD_sequenceNumber: pp->setSequenceNumber(string2long(value)); break;
        case FIELD_LoRaTP: pp->setLoRaTP(string2double(value)); break;
        case FIELD_LoRaCF: pp->setLoRaCF(Hz(string2double(value))); break;
        case FIELD_LoRaSF: pp->setLoRaSF(string2long(value)); break;
        case FIELD_LoRaBW: pp->setLoRaBW(Hz(string2double(value))); break;
        case FIELD_LoRaCR: pp->setLoRaCR(string2long(value)); break;
        case FIELD_LoRaUseHeader: pp->setLoRaUseHeader(string2bool(value)); break;
        case FIELD_RSSI: pp->setRSSI(string2double(value)); break;
        case FIELD_SNIR: pp->setSNIR(string2double(value)); break;
        case FIELD_miRSSI: pp->setMiRSSI(string2double(value)); break;
        case FIELD_miSNIR: pp->setMiSNIR(string2double(value)); break;
        case FIELD_isGeneratedByGateway: pp->setIsGeneratedByGateway(string2bool(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaMacFrame'", field);
    }
}

const char *LoRaMacFrameDescriptor::getFieldStructName(int field) const
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

void *LoRaMacFrameDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    LoRaMacFrame *pp = (LoRaMacFrame *)object; (void)pp;
    switch (field) {
        case FIELD_transmitterAddress: return toVoidPtr(&pp->getTransmitterAddress()); break;
        case FIELD_receiverAddress: return toVoidPtr(&pp->getReceiverAddress()); break;
        default: return nullptr;
    }
}

void LoRaMacFrameDescriptor::setFieldStructValuePointer(void *object, int field, int i, void *ptr) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaMacFrame *pp = (LoRaMacFrame *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaMacFrame'", field);
    }
}

} // namespace flora

