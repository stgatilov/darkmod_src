
//******************************************************************************
// RCF - Remote Call Framework
// Copyright (c) 2005 - 2007. All rights reserved.
// Consult your license for conditions of use.
// Developed by Jarl Lindrud.
// Contact: jlindrud@hotmail.com .
//******************************************************************************

#ifndef INCLUDE_RCF_FILTERSERVICE_HPP
#define INCLUDE_RCF_FILTERSERVICE_HPP

#include <map>

#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include <RCF/AsyncFilter.hpp>
#include <RCF/Service.hpp>

namespace RCF {

    class Session;

    /// Service enabling RcfServer to supply its transports with filters, e.g. for compression and encryption.
    class FilterService :
        public I_Service,
        public I_FilterFactoryLookupProvider, boost::noncopyable
    {
    public:
        /// Constructor.
        FilterService();
       
        /// Adds a filter factory to the service.
        /// \param filterFactoryPtr Filter factory to add.
        void addFilterFactory(FilterFactoryPtr filterFactoryPtr);

        /// Adds a filter factory to the service.
        /// \param filterFactoryPtr Filter factory to add.
        void addFilterFactory(
            FilterFactoryPtr filterFactoryPtr,
            const std::vector<int> &filterIds);

        /// Remotely accessible (via I_RequestTransportFilters), allows clients to request transport and payload filters,
        /// for their communication with the server.
        /// \param filterIds Vector of integers indicating the desired sequence of filters.
        /// \return true if successful, false otherwise. If true, then subsequent communication between server and client will be filtered as requested.
        boost::int32_t requestTransportFilters(const std::vector<boost::int32_t> &filterIds);

        boost::int32_t queryForTransportFilters(const std::vector<boost::int32_t> &filterIds);

        /// Returns a filter factory for the given filter id.
        /// \param filterId Filter id.
        /// \return Filter factory for given filter id (null if filter id is not recognized).
        FilterFactoryPtr getFilterFactoryPtr(int filterId);
   
    private:
        void setTransportFilters(
            RcfSession &session,
            boost::shared_ptr<std::vector<FilterPtr> > filters);

        void onServiceAdded(RcfServer &server);
        void onServiceRemoved(RcfServer &server);
        typedef std::map<int, FilterFactoryPtr> FilterFactoryMap; // TODO: synchronization and documentation
        FilterFactoryMap mFilterFactoryMap;
        ReadWriteMutex mFilterFactoryMapMutex;
    };

    typedef boost::shared_ptr<FilterService> FilterServicePtr;

} // namespace RCF

#endif // ! INCLUDE_RCF_FILTERSERVICE_HPP
