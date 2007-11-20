
#include <SF/Serializer.hpp>

//#include <SF/Exception.hpp>
#include <SF/Node.hpp>

#include <RCF/ClientStub.hpp>
#include <RCF/RcfSession.hpp>
#include <RCF/Tools.hpp>

namespace SF {

    bool SerializerBase::invokeRead(Archive &ar)
    {
        Node node( "", "", 0, false );
        Node *pNode = NULL;

        if (ar.isFlagSet(Archive::NODE_ALREADY_READ))
        {
            I_LocalStorage &localStorage =
                dynamic_cast<I_WithLocalStorage *>(ar.getStream())->getLocalStorage();
            pNode = reinterpret_cast<Node *>(localStorage.getNode());
        }
        else
        {
            if (! dynamic_cast< WithFormatRead * >(ar.getStream())->begin(node))
                return false;
            pNode = &node;
        }

        // Detect polymorphism, either through pointers or through references
        if (!ar.isFlagSet(Archive::POLYMORPHIC))
        {
            if ((ar.isFlagSet(Archive::POINTER)) ||
                (isDerived() && (!ar.isFlagSet(Archive::PARENT))))
            {
                if (pNode->type.length() > 0)
                {
                    ar.setFlag(Archive::POLYMORPHIC, true );
                    std::string derivedTypeName = pNode->type.cpp_str();
                    getSerializerPolymorphic(derivedTypeName);
                    dynamic_cast<I_WithLocalStorage *>(ar.getStream())->getLocalStorage().setNode(pNode);
                    ar.setFlag(Archive::NODE_ALREADY_READ);
                    return invokeSerializerPolymorphic(ar);
                }
            }
        }

        // May now assume that the object is not polymorphic
        UInt32 nid = pNode->id;
        bool bId = pNode->id ? true : false;
        bool bNode = pNode->ref ? false : true;
        bool bPointer = ar.isFlagSet(Archive::POINTER);
        ar.clearState();

        if (bId && bNode && bPointer)
        {
            newObject(ar);
            addToInputContext(ar.getStream(), nid);
            serializeContents(ar);
        }
        else if ( !bId && bNode && bPointer )
        {
            newObject(ar);
            serializeContents(ar);
        }
        else if (bId && !bNode && bPointer)
        {
            queryInputContext(ar.getStream(), nid);
            setFromId();
        }
        else if (bId && bNode && !bPointer)
        {
            addToInputContext(ar.getStream(), nid);
            serializeContents(ar);
        }
        else if (!bId && bNode && !bPointer )
        {
            serializeContents(ar);
        }
        else if (!bId && !bNode && bPointer)
        {
            setToNull();
        }
        else if (!bId && !bNode && !bPointer)
        {
            RCF_THROW(RCF::Exception(RCF::SfError_DataFormat));
        }
        else if (bId && !bNode && !bPointer)
        {
            RCF_THROW(RCF::Exception(RCF::SfError_RefMismatch));
        }

        dynamic_cast< WithFormatRead * >(ar.getStream())->end();

        return true;
    }

    bool SerializerBase::invokeWrite(Archive &ar)
    {
        Node in("", "", 0, false);

        // Detect polymorphism
        if (isDerived() && !ar.isFlagSet(Archive::PARENT))
        {
            ar.setFlag(Archive::POLYMORPHIC);
            getSerializerPolymorphic(getDerivedTypeName());
            return invokeSerializerPolymorphic(ar);
        }

        // May now assume non-polymorphic object

        if (ar.isFlagSet(Archive::POLYMORPHIC))
        {
            in.type.assign(getTypeName());
        }

        bool bPointer = ar.isFlagSet(Archive::POINTER);
        bool bNonAtomic = isNonAtomic();

        if (isNull())
        {
            in.id = 0;
            in.ref = 1;
            dynamic_cast<WithFormatWrite*>(ar.getStream())->begin(in);
            ar.clearState();
        }
        else if (bPointer || bNonAtomic)
        {
            if (queryOutputContext(ar.getStream(), in.id ))
            {
                if (bPointer)
                    in.ref = 1;
                dynamic_cast<WithFormatWrite*>(ar.getStream())->begin(in);
                ar.clearState();
                if (!bPointer)
                    serializeContents(ar);
            }
            else
            {
                addToOutputContext(ar.getStream(), in.id);
                dynamic_cast<WithFormatWrite*>(ar.getStream())->begin(in);
                ar.clearState();
                serializeContents(ar);
            }
        }
        else
        {
            dynamic_cast<WithFormatWrite*>(ar.getStream())->begin(in);
            ar.clearState();
            serializeContents(ar);
        }

        dynamic_cast<WithFormatWrite*>(ar.getStream())->end();

        return true;
    }

    SerializerBase::SerializerBase()
    {}

    SerializerBase::~SerializerBase()
    {}

    bool SerializerBase::invoke(Archive &ar)
    {
        if (ar.isFlagSet(Archive::NO_BEGIN_END))
        {
            ar.clearFlag(Archive::NO_BEGIN_END);
            serializeContents(ar);
            return true;
        }
        else
        {
            RCF_ASSERT( ar.isRead() ||ar.isWrite() );
            if (ar.isRead())
            {
                return invokeRead(ar);
            }
            else //if (ar.isWrite())
            {
                return invokeWrite(ar);
            }
        }
    }

    int getMyRuntimeVersion()
    {
        RCF::ClientStubPtr clientStubPtr = RCF::getCurrentClientStubPtr();
        RCF::RcfSessionPtr rcfSessionPtr = RCF::getCurrentRcfSessionPtr();
        if (clientStubPtr)
        {
            return clientStubPtr->getRcfRuntimeVersion();
        }
        else if (rcfSessionPtr)
        {
            return rcfSessionPtr->getRcfRuntimeVersion();
        }
        else
        {
            return RCF::gRcfRuntimeVersion;
        }
    }

} // namespace SF
