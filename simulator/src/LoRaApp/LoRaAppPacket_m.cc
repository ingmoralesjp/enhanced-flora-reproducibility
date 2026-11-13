//
// Generated file, do not edit! Created by nedtool 6.0 from LoRaApp/LoRaAppPacket.msg.
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
#include "LoRaAppPacket_m.h"

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

EXECUTE_ON_STARTUP(
    omnetpp::cEnum *e = omnetpp::cEnum::find("flora::AppPacketType");
    if (!e) omnetpp::enums.getInstance()->add(e = new omnetpp::cEnum("flora::AppPacketType"));
    e->insert(JOIN_REQUEST, "JOIN_REQUEST");
    e->insert(JOIN_REPLY, "JOIN_REPLY");
    e->insert(DATA, "DATA");
    e->insert(TXCONFIG, "TXCONFIG");
    e->insert(ACK, "ACK");
)

EXECUTE_ON_STARTUP(
    omnetpp::cEnum *e = omnetpp::cEnum::find("flora::AdrMobilityClass");
    if (!e) omnetpp::enums.getInstance()->add(e = new omnetpp::cEnum("flora::AdrMobilityClass"));
    e->insert(ADR_MOBILITY_UNKNOWN, "ADR_MOBILITY_UNKNOWN");
    e->insert(ADR_MOBILITY_STATIC, "ADR_MOBILITY_STATIC");
    e->insert(ADR_MOBILITY_MOBILE, "ADR_MOBILITY_MOBILE");
)

LoRaOptions::LoRaOptions()
{
}

LoRaOptions::LoRaOptions(const LoRaOptions& other)
{
    copy(other);
}

LoRaOptions::~LoRaOptions()
{
}

LoRaOptions& LoRaOptions::operator=(const LoRaOptions& other)
{
    if (this == &other) return *this;
    copy(other);
    return *this;
}

void LoRaOptions::copy(const LoRaOptions& other)
{
    this->LoRaTP = other.LoRaTP;
    this->LoRaCF = other.LoRaCF;
    this->LoRaSF = other.LoRaSF;
    this->LoRaBW = other.LoRaBW;
    this->LoRaCR = other.LoRaCR;
    this->UseHeader = other.UseHeader;
    this->ADRACKReq = other.ADRACKReq;
    this->packetCounter = other.packetCounter;
    this->distanceED = other.distanceED;
    this->positionX = other.positionX;
    this->positionY = other.positionY;
    this->hasPosition = other.hasPosition;
    this->sentAt = other.sentAt;
    this->isHeartbeat_ = other.isHeartbeat_;
    this->isconfirmedMSG = other.isconfirmedMSG;
    this->heartbeatInterval = other.heartbeatInterval;
    this->mobilityClass = other.mobilityClass;
    this->feedbackSnr = other.feedbackSnr;
}

void LoRaOptions::parsimPack(omnetpp::cCommBuffer *b) const
{
    doParsimPacking(b,this->LoRaTP);
    doParsimPacking(b,this->LoRaCF);
    doParsimPacking(b,this->LoRaSF);
    doParsimPacking(b,this->LoRaBW);
    doParsimPacking(b,this->LoRaCR);
    doParsimPacking(b,this->UseHeader);
    doParsimPacking(b,this->ADRACKReq);
    doParsimPacking(b,this->packetCounter);
    doParsimPacking(b,this->distanceED);
    doParsimPacking(b,this->positionX);
    doParsimPacking(b,this->positionY);
    doParsimPacking(b,this->hasPosition);
    doParsimPacking(b,this->sentAt);
    doParsimPacking(b,this->isHeartbeat_);
    doParsimPacking(b,this->isconfirmedMSG);
    doParsimPacking(b,this->heartbeatInterval);
    doParsimPacking(b,this->mobilityClass);
    doParsimPacking(b,this->feedbackSnr);
}

void LoRaOptions::parsimUnpack(omnetpp::cCommBuffer *b)
{
    doParsimUnpacking(b,this->LoRaTP);
    doParsimUnpacking(b,this->LoRaCF);
    doParsimUnpacking(b,this->LoRaSF);
    doParsimUnpacking(b,this->LoRaBW);
    doParsimUnpacking(b,this->LoRaCR);
    doParsimUnpacking(b,this->UseHeader);
    doParsimUnpacking(b,this->ADRACKReq);
    doParsimUnpacking(b,this->packetCounter);
    doParsimUnpacking(b,this->distanceED);
    doParsimUnpacking(b,this->positionX);
    doParsimUnpacking(b,this->positionY);
    doParsimUnpacking(b,this->hasPosition);
    doParsimUnpacking(b,this->sentAt);
    doParsimUnpacking(b,this->isHeartbeat_);
    doParsimUnpacking(b,this->isconfirmedMSG);
    doParsimUnpacking(b,this->heartbeatInterval);
    doParsimUnpacking(b,this->mobilityClass);
    doParsimUnpacking(b,this->feedbackSnr);
}

double LoRaOptions::getLoRaTP() const
{
    return this->LoRaTP;
}

void LoRaOptions::setLoRaTP(double LoRaTP)
{
    this->LoRaTP = LoRaTP;
}

double LoRaOptions::getLoRaCF() const
{
    return this->LoRaCF;
}

void LoRaOptions::setLoRaCF(double LoRaCF)
{
    this->LoRaCF = LoRaCF;
}

int LoRaOptions::getLoRaSF() const
{
    return this->LoRaSF;
}

void LoRaOptions::setLoRaSF(int LoRaSF)
{
    this->LoRaSF = LoRaSF;
}

double LoRaOptions::getLoRaBW() const
{
    return this->LoRaBW;
}

void LoRaOptions::setLoRaBW(double LoRaBW)
{
    this->LoRaBW = LoRaBW;
}

int LoRaOptions::getLoRaCR() const
{
    return this->LoRaCR;
}

void LoRaOptions::setLoRaCR(int LoRaCR)
{
    this->LoRaCR = LoRaCR;
}

bool LoRaOptions::getUseHeader() const
{
    return this->UseHeader;
}

void LoRaOptions::setUseHeader(bool UseHeader)
{
    this->UseHeader = UseHeader;
}

bool LoRaOptions::getADRACKReq() const
{
    return this->ADRACKReq;
}

void LoRaOptions::setADRACKReq(bool ADRACKReq)
{
    this->ADRACKReq = ADRACKReq;
}

int LoRaOptions::getPacketCounter() const
{
    return this->packetCounter;
}

void LoRaOptions::setPacketCounter(int packetCounter)
{
    this->packetCounter = packetCounter;
}

double LoRaOptions::getDistanceED() const
{
    return this->distanceED;
}

void LoRaOptions::setDistanceED(double distanceED)
{
    this->distanceED = distanceED;
}

double LoRaOptions::getPositionX() const
{
    return this->positionX;
}

void LoRaOptions::setPositionX(double positionX)
{
    this->positionX = positionX;
}

double LoRaOptions::getPositionY() const
{
    return this->positionY;
}

void LoRaOptions::setPositionY(double positionY)
{
    this->positionY = positionY;
}

bool LoRaOptions::getHasPosition() const
{
    return this->hasPosition;
}

void LoRaOptions::setHasPosition(bool hasPosition)
{
    this->hasPosition = hasPosition;
}

omnetpp::simtime_t LoRaOptions::getSentAt() const
{
    return this->sentAt;
}

void LoRaOptions::setSentAt(omnetpp::simtime_t sentAt)
{
    this->sentAt = sentAt;
}

bool LoRaOptions::isHeartbeat() const
{
    return this->isHeartbeat_;
}

void LoRaOptions::setIsHeartbeat(bool isHeartbeat)
{
    this->isHeartbeat_ = isHeartbeat;
}

bool LoRaOptions::getIsconfirmedMSG() const
{
    return this->isconfirmedMSG;
}

void LoRaOptions::setIsconfirmedMSG(bool isconfirmedMSG)
{
    this->isconfirmedMSG = isconfirmedMSG;
}

double LoRaOptions::getHeartbeatInterval() const
{
    return this->heartbeatInterval;
}

void LoRaOptions::setHeartbeatInterval(double heartbeatInterval)
{
    this->heartbeatInterval = heartbeatInterval;
}

int LoRaOptions::getMobilityClass() const
{
    return this->mobilityClass;
}

void LoRaOptions::setMobilityClass(int mobilityClass)
{
    this->mobilityClass = mobilityClass;
}

double LoRaOptions::getFeedbackSnr() const
{
    return this->feedbackSnr;
}

void LoRaOptions::setFeedbackSnr(double feedbackSnr)
{
    this->feedbackSnr = feedbackSnr;
}

class LoRaOptionsDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_LoRaTP,
        FIELD_LoRaCF,
        FIELD_LoRaSF,
        FIELD_LoRaBW,
        FIELD_LoRaCR,
        FIELD_UseHeader,
        FIELD_ADRACKReq,
        FIELD_packetCounter,
        FIELD_distanceED,
        FIELD_positionX,
        FIELD_positionY,
        FIELD_hasPosition,
        FIELD_sentAt,
        FIELD_isHeartbeat,
        FIELD_isconfirmedMSG,
        FIELD_heartbeatInterval,
        FIELD_mobilityClass,
        FIELD_feedbackSnr,
    };
  public:
    LoRaOptionsDescriptor();
    virtual ~LoRaOptionsDescriptor();

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

Register_ClassDescriptor(LoRaOptionsDescriptor)

LoRaOptionsDescriptor::LoRaOptionsDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(flora::LoRaOptions)), "")
{
    propertynames = nullptr;
}

LoRaOptionsDescriptor::~LoRaOptionsDescriptor()
{
    delete[] propertynames;
}

bool LoRaOptionsDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<LoRaOptions *>(obj)!=nullptr;
}

const char **LoRaOptionsDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *LoRaOptionsDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int LoRaOptionsDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 18+basedesc->getFieldCount() : 18;
}

unsigned int LoRaOptionsDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_LoRaTP
        FD_ISEDITABLE,    // FIELD_LoRaCF
        FD_ISEDITABLE,    // FIELD_LoRaSF
        FD_ISEDITABLE,    // FIELD_LoRaBW
        FD_ISEDITABLE,    // FIELD_LoRaCR
        FD_ISEDITABLE,    // FIELD_UseHeader
        FD_ISEDITABLE,    // FIELD_ADRACKReq
        FD_ISEDITABLE,    // FIELD_packetCounter
        FD_ISEDITABLE,    // FIELD_distanceED
        FD_ISEDITABLE,    // FIELD_positionX
        FD_ISEDITABLE,    // FIELD_positionY
        FD_ISEDITABLE,    // FIELD_hasPosition
        0,    // FIELD_sentAt
        FD_ISEDITABLE,    // FIELD_isHeartbeat
        FD_ISEDITABLE,    // FIELD_isconfirmedMSG
        FD_ISEDITABLE,    // FIELD_heartbeatInterval
        FD_ISEDITABLE,    // FIELD_mobilityClass
        FD_ISEDITABLE,    // FIELD_feedbackSnr
    };
    return (field >= 0 && field < 18) ? fieldTypeFlags[field] : 0;
}

const char *LoRaOptionsDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "LoRaTP",
        "LoRaCF",
        "LoRaSF",
        "LoRaBW",
        "LoRaCR",
        "UseHeader",
        "ADRACKReq",
        "packetCounter",
        "distanceED",
        "positionX",
        "positionY",
        "hasPosition",
        "sentAt",
        "isHeartbeat",
        "isconfirmedMSG",
        "heartbeatInterval",
        "mobilityClass",
        "feedbackSnr",
    };
    return (field >= 0 && field < 18) ? fieldNames[field] : nullptr;
}

int LoRaOptionsDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaTP") == 0) return base+0;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaCF") == 0) return base+1;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaSF") == 0) return base+2;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaBW") == 0) return base+3;
    if (fieldName[0] == 'L' && strcmp(fieldName, "LoRaCR") == 0) return base+4;
    if (fieldName[0] == 'U' && strcmp(fieldName, "UseHeader") == 0) return base+5;
    if (fieldName[0] == 'A' && strcmp(fieldName, "ADRACKReq") == 0) return base+6;
    if (fieldName[0] == 'p' && strcmp(fieldName, "packetCounter") == 0) return base+7;
    if (fieldName[0] == 'd' && strcmp(fieldName, "distanceED") == 0) return base+8;
    if (fieldName[0] == 'p' && strcmp(fieldName, "positionX") == 0) return base+9;
    if (fieldName[0] == 'p' && strcmp(fieldName, "positionY") == 0) return base+10;
    if (fieldName[0] == 'h' && strcmp(fieldName, "hasPosition") == 0) return base+11;
    if (fieldName[0] == 's' && strcmp(fieldName, "sentAt") == 0) return base+12;
    if (fieldName[0] == 'i' && strcmp(fieldName, "isHeartbeat") == 0) return base+13;
    if (fieldName[0] == 'i' && strcmp(fieldName, "isconfirmedMSG") == 0) return base+14;
    if (fieldName[0] == 'h' && strcmp(fieldName, "heartbeatInterval") == 0) return base+15;
    if (fieldName[0] == 'm' && strcmp(fieldName, "mobilityClass") == 0) return base+16;
    if (fieldName[0] == 'f' && strcmp(fieldName, "feedbackSnr") == 0) return base+17;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *LoRaOptionsDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "double",    // FIELD_LoRaTP
        "double",    // FIELD_LoRaCF
        "int",    // FIELD_LoRaSF
        "double",    // FIELD_LoRaBW
        "int",    // FIELD_LoRaCR
        "bool",    // FIELD_UseHeader
        "bool",    // FIELD_ADRACKReq
        "int",    // FIELD_packetCounter
        "double",    // FIELD_distanceED
        "double",    // FIELD_positionX
        "double",    // FIELD_positionY
        "bool",    // FIELD_hasPosition
        "omnetpp::simtime_t",    // FIELD_sentAt
        "bool",    // FIELD_isHeartbeat
        "bool",    // FIELD_isconfirmedMSG
        "double",    // FIELD_heartbeatInterval
        "int",    // FIELD_mobilityClass
        "double",    // FIELD_feedbackSnr
    };
    return (field >= 0 && field < 18) ? fieldTypeStrings[field] : nullptr;
}

const char **LoRaOptionsDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_mobilityClass: {
            static const char *names[] = { "enum", "enum",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *LoRaOptionsDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_mobilityClass:
            if (!strcmp(propertyname, "enum")) return "AdrMobilityClass";
            if (!strcmp(propertyname, "enum")) return "flora::AdrMobilityClass";
            return nullptr;
        default: return nullptr;
    }
}

int LoRaOptionsDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    LoRaOptions *pp = (LoRaOptions *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

void LoRaOptionsDescriptor::setFieldArraySize(void *object, int field, int size) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldArraySize(object, field, size);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaOptions *pp = (LoRaOptions *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'LoRaOptions'", field);
    }
}

const char *LoRaOptionsDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaOptions *pp = (LoRaOptions *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string LoRaOptionsDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaOptions *pp = (LoRaOptions *)object; (void)pp;
    switch (field) {
        case FIELD_LoRaTP: return double2string(pp->getLoRaTP());
        case FIELD_LoRaCF: return double2string(pp->getLoRaCF());
        case FIELD_LoRaSF: return long2string(pp->getLoRaSF());
        case FIELD_LoRaBW: return double2string(pp->getLoRaBW());
        case FIELD_LoRaCR: return long2string(pp->getLoRaCR());
        case FIELD_UseHeader: return bool2string(pp->getUseHeader());
        case FIELD_ADRACKReq: return bool2string(pp->getADRACKReq());
        case FIELD_packetCounter: return long2string(pp->getPacketCounter());
        case FIELD_distanceED: return double2string(pp->getDistanceED());
        case FIELD_positionX: return double2string(pp->getPositionX());
        case FIELD_positionY: return double2string(pp->getPositionY());
        case FIELD_hasPosition: return bool2string(pp->getHasPosition());
        case FIELD_sentAt: return simtime2string(pp->getSentAt());
        case FIELD_isHeartbeat: return bool2string(pp->isHeartbeat());
        case FIELD_isconfirmedMSG: return bool2string(pp->getIsconfirmedMSG());
        case FIELD_heartbeatInterval: return double2string(pp->getHeartbeatInterval());
        case FIELD_mobilityClass: return enum2string(pp->getMobilityClass(), "flora::AdrMobilityClass");
        case FIELD_feedbackSnr: return double2string(pp->getFeedbackSnr());
        default: return "";
    }
}

void LoRaOptionsDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaOptions *pp = (LoRaOptions *)object; (void)pp;
    switch (field) {
        case FIELD_LoRaTP: pp->setLoRaTP(string2double(value)); break;
        case FIELD_LoRaCF: pp->setLoRaCF(string2double(value)); break;
        case FIELD_LoRaSF: pp->setLoRaSF(string2long(value)); break;
        case FIELD_LoRaBW: pp->setLoRaBW(string2double(value)); break;
        case FIELD_LoRaCR: pp->setLoRaCR(string2long(value)); break;
        case FIELD_UseHeader: pp->setUseHeader(string2bool(value)); break;
        case FIELD_ADRACKReq: pp->setADRACKReq(string2bool(value)); break;
        case FIELD_packetCounter: pp->setPacketCounter(string2long(value)); break;
        case FIELD_distanceED: pp->setDistanceED(string2double(value)); break;
        case FIELD_positionX: pp->setPositionX(string2double(value)); break;
        case FIELD_positionY: pp->setPositionY(string2double(value)); break;
        case FIELD_hasPosition: pp->setHasPosition(string2bool(value)); break;
        case FIELD_isHeartbeat: pp->setIsHeartbeat(string2bool(value)); break;
        case FIELD_isconfirmedMSG: pp->setIsconfirmedMSG(string2bool(value)); break;
        case FIELD_heartbeatInterval: pp->setHeartbeatInterval(string2double(value)); break;
        case FIELD_mobilityClass: pp->setMobilityClass((flora::AdrMobilityClass)string2enum(value, "flora::AdrMobilityClass")); break;
        case FIELD_feedbackSnr: pp->setFeedbackSnr(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaOptions'", field);
    }
}

const char *LoRaOptionsDescriptor::getFieldStructName(int field) const
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

void *LoRaOptionsDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    LoRaOptions *pp = (LoRaOptions *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

void LoRaOptionsDescriptor::setFieldStructValuePointer(void *object, int field, int i, void *ptr) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaOptions *pp = (LoRaOptions *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaOptions'", field);
    }
}

Register_Class(LoRaAppPacket)

LoRaAppPacket::LoRaAppPacket() : ::inet::FieldsChunk()
{
}

LoRaAppPacket::LoRaAppPacket(const LoRaAppPacket& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

LoRaAppPacket::~LoRaAppPacket()
{
}

LoRaAppPacket& LoRaAppPacket::operator=(const LoRaAppPacket& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void LoRaAppPacket::copy(const LoRaAppPacket& other)
{
    this->msgType = other.msgType;
    this->sampleMeasurement = other.sampleMeasurement;
    this->options = other.options;
}

void LoRaAppPacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->msgType);
    doParsimPacking(b,this->sampleMeasurement);
    doParsimPacking(b,this->options);
}

void LoRaAppPacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->msgType);
    doParsimUnpacking(b,this->sampleMeasurement);
    doParsimUnpacking(b,this->options);
}

int LoRaAppPacket::getMsgType() const
{
    return this->msgType;
}

void LoRaAppPacket::setMsgType(int msgType)
{
    handleChange();
    this->msgType = msgType;
}

int LoRaAppPacket::getSampleMeasurement() const
{
    return this->sampleMeasurement;
}

void LoRaAppPacket::setSampleMeasurement(int sampleMeasurement)
{
    handleChange();
    this->sampleMeasurement = sampleMeasurement;
}

const LoRaOptions& LoRaAppPacket::getOptions() const
{
    return this->options;
}

void LoRaAppPacket::setOptions(const LoRaOptions& options)
{
    handleChange();
    this->options = options;
}

class LoRaAppPacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertynames;
    enum FieldConstants {
        FIELD_msgType,
        FIELD_sampleMeasurement,
        FIELD_options,
    };
  public:
    LoRaAppPacketDescriptor();
    virtual ~LoRaAppPacketDescriptor();

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

Register_ClassDescriptor(LoRaAppPacketDescriptor)

LoRaAppPacketDescriptor::LoRaAppPacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(flora::LoRaAppPacket)), "inet::FieldsChunk")
{
    propertynames = nullptr;
}

LoRaAppPacketDescriptor::~LoRaAppPacketDescriptor()
{
    delete[] propertynames;
}

bool LoRaAppPacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<LoRaAppPacket *>(obj)!=nullptr;
}

const char **LoRaAppPacketDescriptor::getPropertyNames() const
{
    if (!propertynames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
        const char **basenames = basedesc ? basedesc->getPropertyNames() : nullptr;
        propertynames = mergeLists(basenames, names);
    }
    return propertynames;
}

const char *LoRaAppPacketDescriptor::getProperty(const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : nullptr;
}

int LoRaAppPacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 3+basedesc->getFieldCount() : 3;
}

unsigned int LoRaAppPacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeFlags(field);
        field -= basedesc->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_msgType
        FD_ISEDITABLE,    // FIELD_sampleMeasurement
        FD_ISCOMPOUND,    // FIELD_options
    };
    return (field >= 0 && field < 3) ? fieldTypeFlags[field] : 0;
}

const char *LoRaAppPacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldName(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldNames[] = {
        "msgType",
        "sampleMeasurement",
        "options",
    };
    return (field >= 0 && field < 3) ? fieldNames[field] : nullptr;
}

int LoRaAppPacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount() : 0;
    if (fieldName[0] == 'm' && strcmp(fieldName, "msgType") == 0) return base+0;
    if (fieldName[0] == 's' && strcmp(fieldName, "sampleMeasurement") == 0) return base+1;
    if (fieldName[0] == 'o' && strcmp(fieldName, "options") == 0) return base+2;
    return basedesc ? basedesc->findField(fieldName) : -1;
}

const char *LoRaAppPacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldTypeString(field);
        field -= basedesc->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_msgType
        "int",    // FIELD_sampleMeasurement
        "flora::LoRaOptions",    // FIELD_options
    };
    return (field >= 0 && field < 3) ? fieldTypeStrings[field] : nullptr;
}

const char **LoRaAppPacketDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldPropertyNames(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_msgType: {
            static const char *names[] = { "enum", "enum",  nullptr };
            return names;
        }
        default: return nullptr;
    }
}

const char *LoRaAppPacketDescriptor::getFieldProperty(int field, const char *propertyname) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldProperty(field, propertyname);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_msgType:
            if (!strcmp(propertyname, "enum")) return "AppPacketType";
            if (!strcmp(propertyname, "enum")) return "flora::AppPacketType";
            return nullptr;
        default: return nullptr;
    }
}

int LoRaAppPacketDescriptor::getFieldArraySize(void *object, int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldArraySize(object, field);
        field -= basedesc->getFieldCount();
    }
    LoRaAppPacket *pp = (LoRaAppPacket *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

void LoRaAppPacketDescriptor::setFieldArraySize(void *object, int field, int size) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldArraySize(object, field, size);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaAppPacket *pp = (LoRaAppPacket *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'LoRaAppPacket'", field);
    }
}

const char *LoRaAppPacketDescriptor::getFieldDynamicTypeString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldDynamicTypeString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaAppPacket *pp = (LoRaAppPacket *)object; (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string LoRaAppPacketDescriptor::getFieldValueAsString(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldValueAsString(object,field,i);
        field -= basedesc->getFieldCount();
    }
    LoRaAppPacket *pp = (LoRaAppPacket *)object; (void)pp;
    switch (field) {
        case FIELD_msgType: return enum2string(pp->getMsgType(), "flora::AppPacketType");
        case FIELD_sampleMeasurement: return long2string(pp->getSampleMeasurement());
        case FIELD_options: {std::stringstream out; out << pp->getOptions(); return out.str();}
        default: return "";
    }
}

void LoRaAppPacketDescriptor::setFieldValueAsString(void *object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaAppPacket *pp = (LoRaAppPacket *)object; (void)pp;
    switch (field) {
        case FIELD_msgType: pp->setMsgType((flora::AppPacketType)string2enum(value, "flora::AppPacketType")); break;
        case FIELD_sampleMeasurement: pp->setSampleMeasurement(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaAppPacket'", field);
    }
}

const char *LoRaAppPacketDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructName(field);
        field -= basedesc->getFieldCount();
    }
    switch (field) {
        case FIELD_options: return omnetpp::opp_typename(typeid(LoRaOptions));
        default: return nullptr;
    };
}

void *LoRaAppPacketDescriptor::getFieldStructValuePointer(void *object, int field, int i) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount())
            return basedesc->getFieldStructValuePointer(object, field, i);
        field -= basedesc->getFieldCount();
    }
    LoRaAppPacket *pp = (LoRaAppPacket *)object; (void)pp;
    switch (field) {
        case FIELD_options: return toVoidPtr(&pp->getOptions()); break;
        default: return nullptr;
    }
}

void LoRaAppPacketDescriptor::setFieldStructValuePointer(void *object, int field, int i, void *ptr) const
{
    omnetpp::cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount()) {
            basedesc->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= basedesc->getFieldCount();
    }
    LoRaAppPacket *pp = (LoRaAppPacket *)object; (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'LoRaAppPacket'", field);
    }
}

} // namespace flora

