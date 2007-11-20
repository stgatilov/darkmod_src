
#include <SF/Stream.hpp>

#include <RCF/ByteOrdering.hpp>
#include <SF/DataPtr.hpp>
#include <SF/Encoding.hpp>
#include <SF/Exception.hpp>
#include <SF/Node.hpp>
#include <SF/Tools.hpp>

#include <vector>

namespace SF {

    // ContextRead

    ContextRead::ContextRead() :
        bEnabled_(true)
    {}

    ContextRead::~ContextRead()
    {}

    void ContextRead::add(UInt32 nid, const ObjectId &id)
    {
        if (bEnabled_)
        {
            if (nid_id_ptr_.get() == NULL)
            {
                nid_id_ptr_.reset( new std::map<UInt32, ObjectId>());
            }
            (*nid_id_ptr_)[nid] = id;
        }
    }

    void ContextRead::add(void *ptr, const std::type_info &objType, void *pObj)
    {
        if (bEnabled_)
        {
            if (type_ptr_obj_ptr_.get() == NULL)
            {
                type_ptr_obj_ptr_.reset( new std::map<std::string, std::map< void *, void * > >() );
            }
            (*type_ptr_obj_ptr_)[ objType.name() ][ ptr ] = pObj;
        }
    }

    bool ContextRead::query(UInt32 nid, ObjectId &id)
    {
        if (nid_id_ptr_.get() && nid_id_ptr_->find( nid ) != nid_id_ptr_->end())
        {
            id = (*nid_id_ptr_)[ nid ];
            return true;
        }
        else
        {
            return false;
        }
    }

    bool ContextRead::query(void *ptr, const std::type_info &objType, void *&pObj)
    {
        if (type_ptr_obj_ptr_.get() &&
            (*type_ptr_obj_ptr_)[ objType.name() ].find(ptr) != (*type_ptr_obj_ptr_)[ objType.name() ].end() )
        {
            pObj = (*type_ptr_obj_ptr_)[ objType.name() ][ ptr ];
            return true;
        }
        else
        {
            return false;
        }
    }

    void ContextRead::clear()
    {
        if (nid_id_ptr_.get())
        {
            nid_id_ptr_->clear();
        }
        if (type_ptr_obj_ptr_.get())
        {
            type_ptr_obj_ptr_->clear();
        }
    }

    // ContextWrite

    ContextWrite::ContextWrite() :
        bEnabled_(false),
        currentId_(1)
    {}

    ContextWrite::~ContextWrite()
    {}

    void ContextWrite::setEnabled(bool enabled)
    {
        bEnabled_ = enabled;
        if (bEnabled_ && id_nid_ptr_.get() == NULL)
        {
            id_nid_ptr_.reset( new std::map<ObjectId, UInt32>());
        }
    }

    bool ContextWrite::getEnabled()
    {
        return bEnabled_;
    }

    void ContextWrite::add(const ObjectId &id, UInt32 &nid)
    {
        if (bEnabled_)
        {
            if (id_nid_ptr_->find( id ) != id_nid_ptr_->end())
            {
                nid = (*id_nid_ptr_)[ id ];
            }
            else
            {
                nid = (*id_nid_ptr_)[ id ] = currentId_++;
            }
        }
    }
    bool ContextWrite::query(const ObjectId &id, UInt32 &nid)
    {
        if ( bEnabled_ && id_nid_ptr_->find( id ) != id_nid_ptr_->end() )
        {
            nid = (*id_nid_ptr_)[ id ];
            return true;
        }
        else
        {
            return false;
        }
    }

    void ContextWrite::clear()
    {
        if (id_nid_ptr_.get())
        {
            id_nid_ptr_->clear();
        }
        currentId_ = 1;
    }

    // WithContextRead

    WithContextRead::~WithContextRead()
    {}

    I_ContextRead &WithContextRead::getContext()
    {
        return context;
    }

    // WithContextWrite

    WithContextWrite::~WithContextWrite()
    {}

    I_ContextWrite &WithContextWrite::getContext()
    {
        return context;
    }

    // LocalStorage

    LocalStorage::LocalStorage() :
        pNode_(RCF_DEFAULT_INIT)
    {}

    LocalStorage::~LocalStorage()
    {}

    void LocalStorage::setNode(Node *pNode)
    {
        pNode_ = pNode;
    }

    Node *LocalStorage::getNode()
    {
        return pNode_;
    }

    // WithLocalStorage

    WithLocalStorage::~WithLocalStorage()
    {}

    I_LocalStorage &WithLocalStorage::getLocalStorage()
    {
        return localStorage_;
    }


    enum {
        BEGIN,
        DATA,
        END,
        BLANK
    };


    // IStream

    IStream::IStream() :
        pIs(RCF_DEFAULT_INIT),
        pIstr(RCF_DEFAULT_INIT)
    {
    }

    IStream::IStream(std::istream &is) :
        pIs(RCF_DEFAULT_INIT),
        pIstr(RCF_DEFAULT_INIT)
    {
        setIs(is);
    }

    bool IStream::begin(Node &node)
    {
        Byte8 byte = 0;
        read_byte(byte);
        if (byte != BEGIN)
        {
            if (byte == BLANK)
            {
                Byte8 count = 0;
                read_byte(count);
                std::vector<Byte8> buffer(count);
                UInt32 bytesRead = read( &(buffer[0]), count);
                if (bytesRead != static_cast<UInt32>(count))
                {
                    RCF_THROW(RCF::Exception(RCF::SfError_DataFormat))(bytesRead)(count);
                }
                return begin(node);
            }
            else
            {
                putback_byte(byte);
                if (byte != END)
                {
                    RCF_THROW(RCF::Exception(RCF::SfError_DataFormat))(byte);
                }
            }
        }
        else
        {
            read_byte( byte );
            Byte8 attrSpec = byte;

            // id
            if (attrSpec & 1)
            {
                read_int(node.id);
            }

            // ref
            attrSpec = attrSpec >> 1;
            if (attrSpec & 1)
            {
                node.ref = 1;
            }

            // type
            attrSpec = attrSpec >> 1;
            if (attrSpec & 1)
            {
                UInt32 length = 0;
                read_int(length);
                node.type.allocate(length);
                read(node.type.get(), length );
            }

            // label
            attrSpec = attrSpec >> 1;
            if (attrSpec & 1)
            {
                UInt32 length = 0;
                read_int(length);
                node.label.allocate(length);
                read(node.label.get(), length);
            }

        }

        return true;
    }

    bool IStream::get(DataPtr &value)
    {
        Byte8 byte;
        read_byte( byte );
        if (byte == DATA)
        {
            UInt32 length = 0;
            read_int(length);
            value.allocate(length);
            read(value.get(), length);
            return true;
        }
        else
        {
            putback_byte(byte);
            return false;
        }
    }

    void IStream::end()
    {
        Byte8 byte;
        read_byte(byte);
        if (byte != END)
        {
            RCF_THROW(RCF::Exception(RCF::SfError_DataFormat, "no end symbol"))(byte);
        }
    }

    UInt32 IStream::read_int(UInt32 &n)
    {
        static const int nBytes = sizeof(UInt32);
        Byte8 pBytes[nBytes];
        memset(pBytes, 0, nBytes);
        UInt32 bytesRead = 0;
        bytesRead += read(pBytes, nBytes);
        RCF::networkToMachineOrder(pBytes, 4, 1);
        n = * (UInt32 *) pBytes;
        return bytesRead;
    }

    UInt32 IStream::read_byte(Byte8 &byte)
    {
        UInt32 bytesRead = 0;
        Byte8 byte_ = 0;
        bytesRead += read(&byte_, 1);
        byte = * ((Byte8 *) &byte_);
        return bytesRead;
    }

    UInt32 IStream::read(Byte8 *pBytes, UInt32 nLength)
    {
        pIs->read(pBytes, nLength);
        if (pIs->fail())
        {
            RCF_THROW(RCF::Exception(RCF::SfError_ReadFailure))(nLength)(pIs->gcount());
        }
        return static_cast<UInt32>(pIs->gcount());
    }

    bool IStream::supportsReadRaw()
    {
        return pIstr != NULL;
    }
/*
    UInt32 IStream::readRaw(Byte8 *&pBytes, UInt32 nLength)
    {
        pIstr->rdbuf()->freeze(true);
        pBytes = pIstr->rdbuf()->str();

        int pcount = pIstr->rdbuf()->pcount();
        char *pTemp = pBytes+pcount;


        pIstr->rdbuf()->freeze(false);
        std::size_t newPos = pIstr->rdbuf()->pubseekoff(nLength, std::ios_base::cur, std::ios_base::in);
        if (newPos == -1)
        {
            SF_THROW(Exception, "raw read from std::istrstream failed")(nLength);
        }
        if (pIs->fail())
        {
            SF_THROW(Exception, "fail bit set on std::istream")(nLength);
        }
        return nLength;
    }
*/   
    void IStream::putback_byte( Byte8 byte )
    {
        pIs->putback(byte);
    }

    // OStream

    OStream::OStream() : pOs(RCF_DEFAULT_INIT)
    {
    }

    OStream::OStream(std::ostream &os) : pOs(RCF_DEFAULT_INIT)
    {
        setOs(os);
    }

    void OStream::begin( const Node &node )
    {
        write_byte( (Byte8) BEGIN );

        Byte8 attrSpec = 0;
        if (node.id != 0)
        {
            attrSpec |= 1<<0;
        }
        if (node.ref != 0)
        {
            attrSpec |= 1<<1;
        }
        if (node.type.get() != NULL)
        {
            attrSpec |= 1<<2;
        }
        if (node.label.get() != NULL)
        {
            attrSpec |= 1<<3;
        }

        write_byte( attrSpec );

        if (node.id != 0)
        {
            write_int(node.id);
        }
        if (node.type.get() != NULL)
        {
            write(node.type.get(), node.type.length());
        }
        if (node.label.get() != NULL)
        {
            write(node.label.get(), node.label.length() );
        }
    }

    void OStream::put(const DataPtr &value)
    {
        write_byte( (Byte8) DATA );
        write(value.get(), value.length());
    }

    void OStream::end()
    {
        write_byte( (Byte8) END );
    }

    UInt32 OStream::write_int(UInt32 n)
    {
        BOOST_STATIC_ASSERT( sizeof(n) == 4 );
        RCF::machineToNetworkOrder(&n, 4, 1);
        pOs->write( reinterpret_cast<char*>(&n), 4);
        if (pOs->fail())
        {
            RCF_THROW(RCF::Exception(RCF::SfError_WriteFailure))(n);
        }
        return 4;
    }

    UInt32 OStream::write_byte(Byte8 byte)
    {
        pOs->write(&byte, 1);
        if (pOs->fail())
        {
            RCF_THROW(RCF::Exception(RCF::SfError_WriteFailure));
        }
        return 1;
    }

    UInt32 OStream::write(const Byte8 *pBytes, UInt32 nLength)
    {
        UInt32 bytesWritten = 0;
        bytesWritten += write_int(nLength);
        pOs->write(pBytes, nLength);
        if (pOs->fail())
        {
            RCF_THROW(RCF::Exception(RCF::SfError_WriteFailure))(nLength);
        }
        bytesWritten += nLength;
        return bytesWritten;
    }

    UInt32 OStream::writeRaw(const Byte8 *pBytes, UInt32 nLength)
    {
        pOs->write(pBytes, nLength);
        if (pOs->fail())
        {
            RCF_THROW(RCF::Exception(RCF::SfError_WriteFailure))(nLength);
        }
        return nLength;
    }

} // namespace SF
