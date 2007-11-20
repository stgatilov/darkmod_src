
#ifndef INCLUDE_RCF_SSPIFILTER_HPP
#define INCLUDE_RCF_SSPIFILTER_HPP

#include <memory>

#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/AsyncFilter.hpp>
#include <RCF/Tools.hpp>

#include <RCF/util/Tchar.hpp>

#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif

#include <security.h>
#include <tchar.h>

namespace RCF {

    typedef util::tstring tstring;

    tstring getMyUserName();
    tstring getMyDomain();
    tstring getMyMachineName();

    template<typename T1,typename T2>
    class RecursionState
    {
    public:
        RecursionState() :
            mRecursing(RCF_DEFAULT_INIT),
            mT1(RCF_DEFAULT_INIT),
            mT2(RCF_DEFAULT_INIT)
        {}

        void assign(const T1 &t1, const T2 &t2)
        {
            mT1 = t1;
            mT2 = t2;

            clearArg(t1);
            clearArg(t2);
        }

        void clear()
        {
            mRecursing = false;
            mT1 = T1();
            mT2 = T2();
        }

        bool    mRecursing;
        T1      mT1;
        T2      mT2;

    private:
        void clearArg_(const ByteBuffer &byteBuffer, boost::mpl::true_*)
        {
            const_cast<ByteBuffer &>(byteBuffer).clear();
        }

        template<typename T>
        void clearArg_(const T &, boost::mpl::false_*)
        {}

        template<typename T>
        void clearArg(const T &t)
        {
            typedef typename boost::is_same<T, ByteBuffer>::type type;
            clearArg_(t, (type*) 0);
        }
    };

    class SspiFilterBase;

    typedef boost::shared_ptr<SspiFilterBase> SspiFilterBasePtr;

    class SspiImpersonator
    {
    public:
        SspiImpersonator(SspiFilterBasePtr sspiFilterPtr);
        ~SspiImpersonator();
        bool impersonate();
        void revertToSelf() const;
    private:
        SspiFilterBasePtr mSspiFilterPtr;
    };

    //static const ULONG DefaultSspiContextRequirements = 2079;
    static const ULONG DefaultSspiContextRequirements =
        ISC_REQ_REPLAY_DETECT   |
        ISC_REQ_SEQUENCE_DETECT |
        ISC_REQ_CONFIDENTIALITY |
        ISC_REQ_INTEGRITY       |
        ISC_REQ_DELEGATE        |
        ISC_REQ_MUTUAL_AUTH;

    class SspiFilterBase :
        public IdentityFilter, // TODO: replace with Filter
        public boost::enable_shared_from_this<SspiFilterBase>
    {
    public:

        ~SspiFilterBase();

        enum QualityOfProtection
        {
            None,
            Encryption,
            Integrity
        };

        typedef SspiImpersonator Impersonator;

    protected:

        friend class SspiImpersonator;

        SspiFilterBase(
            const tstring &packageName,
            const tstring &packageList,
            bool server = false);

        SspiFilterBase(
            const tstring &target,
            QualityOfProtection qop,
            ULONG contextRequirements,
            const tstring &packageName,
            const tstring &packageList,
            bool server = false);

        SspiFilterBase(
            const tstring &userName,
            const tstring &password,
            const tstring &domain,
            const tstring &target,
            QualityOfProtection qop,
            ULONG contextRequirements,
            const tstring &packageName,
            const tstring &packageList,
            bool server = false);

        enum Event
        {
            ReadIssued,
            WriteIssued,
            ReadCompleted,
            WriteCompleted
        };

        enum ContextState
        {
            AuthContinue,
            AuthOk,
            AuthOkAck,
            AuthFailed
        };

        enum State
        {
            Ready,
            Reading,
            Writing
        };

        PSecurityFunctionTable getSecurityFunctionTable() const;

        void acquireCredentials(
            const tstring &userName = _T(""),
            const tstring &password = _T(""),
            const tstring &domain = _T(""));
        void freeCredentials();

        void init();
        void deinit();
        void reset();

        void read(const ByteBuffer &byteBuffer, std::size_t bytesRequested);
        void write(const std::vector<ByteBuffer> &byteBuffers);
        void onReadCompleted(const ByteBuffer &byteBuffer, int error);
        void onWriteCompleted(std::size_t bytesTransferred, int error);

        void handleEvent(Event event);
        void readBuffer();
        void writeBuffer();
        void encryptWriteBuffer();
        void decryptReadBuffer();
        bool completeReadBlock();
        bool completeWriteBlock();
        bool completeBlock();
        void resumeUserIo();
        void resizeReadBuffer(std::size_t newSize);
        void resizeWriteBuffer(std::size_t newSize);

        virtual void handleHandshakeEvent() = 0;

    protected:

        const tstring                           mPackageName;
        const tstring                           mPackageList;
        const tstring                           mTarget;
        QualityOfProtection                     mQop;
        ULONG                                    mContextRequirements;

        bool                                    mHaveContext;
        bool                                    mHaveCredentials;
        bool                                    mImplicitCredentials;
        SecPkgInfo                              mPkgInfo;
        CtxtHandle                              mContext;
        CredHandle                              mCredentials;

        ContextState                            mContextState;
        State                                   mPreState;
        State                                   mPostState;
        Event                                   mEvent;
        const bool                              mServer;

        ByteBuffer                              mReadByteBufferOrig;
        ByteBuffer                              mWriteByteBufferOrig;
        std::size_t                             mBytesRequestedOrig;

        ByteBuffer                              mReadByteBuffer;
        boost::shared_ptr<std::vector<char> >   mReadBufferVectorPtr;
        char *                                  mReadBuffer;
        std::size_t                             mReadBufferPos;
        std::size_t                             mReadBufferLen;

        ByteBuffer                              mWriteByteBuffer;
        boost::shared_ptr<std::vector<char> >   mWriteBufferVectorPtr;
        char *                                  mWriteBuffer;
        std::size_t                             mWriteBufferPos;
        std::size_t                             mWriteBufferLen;

        std::vector<ByteBuffer>                 mByteBuffers;
        ByteBuffer                              mTempByteBuffer;

    private:
        bool mLimitRecursion;
        RecursionState<ByteBuffer, int> mRecursionStateRead;
        RecursionState<std::size_t, int> mRecursionStateWrite;
        void onReadCompleted_(const ByteBuffer &byteBuffer, int error);
        void onWriteCompleted_(std::size_t bytesTransferred, int error);

        template<typename StateT, typename Pfn, typename T1, typename T2>
        void applyRecursionLimiter(StateT &state, Pfn pfn, const T1 &t1, const T2 &t2)
        {
            state.assign(t1, t2);
            if (state.mRecursing)
            {
                state.mRecursing = false;
            }
            else
            {
                // gcc295 seems to need the namespace qualifier for make_obj_guard anyway
                using namespace boost::multi_index::detail;
                scope_guard guard = boost::multi_index::detail::make_obj_guard(
                    state,
                    &StateT::clear);
                RCF_UNUSED_VARIABLE(guard);
                while (!state.mRecursing)
                {
                    state.mRecursing = true;
                    (this->*pfn)(state.mT1, state.mT2);
                }
            }
        }

    };

    // server filters

    class SspiServerFilter : public SspiFilterBase
    {
    public:
        SspiServerFilter(const tstring &packageName, const tstring &packageList);

    private:
        bool doHandshake();
        void handleHandshakeEvent();
    };

    class SspiNtlmServerFilter : public SspiServerFilter
    {
    public:
        SspiNtlmServerFilter();
        const FilterDescription & getFilterDescription() const;
        static const FilterDescription & sGetFilterDescription();

        // TODO: should be private
        static const FilterDescription *spFilterDescription;
    };

    class SspiKerberosServerFilter : public SspiServerFilter
    {
    public:
        SspiKerberosServerFilter();
        const FilterDescription & getFilterDescription() const;
        static const FilterDescription & sGetFilterDescription();

        // TODO: should be private
        static const FilterDescription *spFilterDescription;
    };

    class SspiNegotiateServerFilter : public SspiServerFilter
    {
    public:
        SspiNegotiateServerFilter(const tstring &packageList);
        const FilterDescription & getFilterDescription() const;
        static const FilterDescription & sGetFilterDescription();

        // TODO: should be private
        static const FilterDescription *spFilterDescription;
    };

    // filter factories

    class SspiNtlmFilterFactory : public FilterFactory
    {
    public:
        FilterPtr createFilter();
        const FilterDescription & getFilterDescription();
    };

    class SspiKerberosFilterFactory : public FilterFactory
    {
    public:
        FilterPtr createFilter();
        const FilterDescription & getFilterDescription();
    };

    class SspiNegotiateFilterFactory : public FilterFactory
    {
    public:
        SspiNegotiateFilterFactory(const tstring &packageList = _T("Kerberos, NTLM"));
        FilterPtr createFilter();
        const FilterDescription & getFilterDescription();
    private:
        tstring mPackageList;
    };

    // client filters

    class SspiClientFilter : public SspiFilterBase
    {
    public:
        SspiClientFilter(
            const tstring &userName,
            const tstring &password,
            const tstring &domain,
            const tstring &targetName,
            QualityOfProtection qop,
            ULONG contextRequirements,
            const tstring &packageName,
            const tstring &packageList) :
                SspiFilterBase(
                    userName, password, domain,
                    targetName, qop, contextRequirements, packageName, packageList)
        {}

        SspiClientFilter(
            const tstring &targetName,
            QualityOfProtection qop,
            ULONG contextRequirements,
            const tstring &packageName,
            const tstring &packageList) :
            SspiFilterBase(
                targetName, qop, contextRequirements, packageName, packageList)
        {}

    private:
        bool doHandshake();
        void handleHandshakeEvent();
    };

    class SspiNtlmClientFilter : public SspiClientFilter
    {
    public:
        SspiNtlmClientFilter(
            const tstring &userName,
            const tstring &password,
            const tstring &domain,
            QualityOfProtection qop = SspiFilterBase::Encryption,
            ULONG contextRequirements = DefaultSspiContextRequirements) :
                SspiClientFilter(
                    userName, password, domain,
                    _T(""), qop, contextRequirements, _T("NTLM"), _T(""))
        {}

        SspiNtlmClientFilter(
            QualityOfProtection qop = SspiFilterBase::Encryption,
            ULONG contextRequirements = DefaultSspiContextRequirements) :
            SspiClientFilter(
                _T(""), qop, contextRequirements, _T("NTLM"), _T(""))
        {}

        const FilterDescription & getFilterDescription() const
        {
            return sGetFilterDescription();
        }

        static const FilterDescription & sGetFilterDescription()
        {
            return SspiNtlmServerFilter::sGetFilterDescription();
        }

    };

    class SspiKerberosClientFilter : public SspiClientFilter
    {
    public:
        SspiKerberosClientFilter(
            const tstring &userName,
            const tstring &password,
            const tstring &domain,
            const tstring &targetName,
            QualityOfProtection qop = SspiFilterBase::None,
            ULONG contextRequirements = DefaultSspiContextRequirements) :
                SspiClientFilter(
                    userName, password, domain,
                    targetName, qop, contextRequirements, _T("Kerberos"), _T(""))
        {}

        SspiKerberosClientFilter(
            const tstring &targetName,
            QualityOfProtection qop = SspiFilterBase::Encryption,
            ULONG contextRequirements = DefaultSspiContextRequirements) :
                SspiClientFilter(
                    targetName,
                    qop, contextRequirements, _T("Kerberos"), _T(""))
        {}

        const FilterDescription & getFilterDescription() const
        {
            return sGetFilterDescription();
        }

        static const FilterDescription & sGetFilterDescription()
        {
            return SspiKerberosServerFilter::sGetFilterDescription();
        }
    };

    class SspiNegotiateClientFilter : public SspiClientFilter
    {
    public:
        SspiNegotiateClientFilter(
            const tstring &userName,
            const tstring &password,
            const tstring &domain,
            const tstring &targetName,
            QualityOfProtection qop = SspiFilterBase::None,
            ULONG contextRequirements = DefaultSspiContextRequirements) :
                SspiClientFilter(
                    userName, password, domain,
                    targetName, qop, contextRequirements, _T("Negotiate"), _T("Kerberos,NTLM"))
        {}

        SspiNegotiateClientFilter(
            const tstring &targetName,
            QualityOfProtection qop = SspiFilterBase::Encryption,
            ULONG contextRequirements = DefaultSspiContextRequirements) :
                SspiClientFilter(
                    targetName,
                    qop, contextRequirements, _T("Negotiate"), _T("Kerberos,NTLM"))
        {}

        const FilterDescription & getFilterDescription() const
        {
            return sGetFilterDescription();
        }

        static const FilterDescription & sGetFilterDescription()
        {
            return SspiNegotiateServerFilter::sGetFilterDescription();
        }
    };

    typedef SspiNtlmClientFilter        SspiNtlmFilter;
    typedef SspiKerberosClientFilter    SspiKerberosFilter;
    typedef SspiNegotiateClientFilter   SspiNegotiateFilter;

} // namespace RCF

#endif // ! INCLUDE_RCF_SSPIFILTER_HPP
