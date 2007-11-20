
#include <RCF/util/TraceTargetFileRollOver.hpp>

namespace util {

    TraceTargetFileRollOver::TraceTargetFileRollOver(
        const std::string &filename,
        std::size_t maxSize,
        std::size_t frequency) :
            fout( new std::ofstream(filename.c_str())),
            filename(filename),
            counter(0),
            fileCounter(0),
            maxSize(maxSize),
            frequency(frequency)
    {}

    bool TraceTargetFileRollOver::isNull()
    {
        return false;
    }

    std::string TraceTargetFileRollOver::getName()
    {
        return filename;
    }

    void TraceTargetFileRollOver::trace(const std::string &msg)
    {
        Lock lock(m); UNUSED_VARIABLE(lock);
        *fout << msg;
        fout->flush();
        if (++counter % frequency == 0)
        {
            boost::filesystem::path path(filename, boost::filesystem::native);
            if (boost::filesystem::exists(path) && boost::filesystem::file_size(path) > maxSize)
            {
                // roll over
                try
                {
                    fout.reset();
                    std::size_t pos = filename.find_last_of('.');
                    std::string s1 = filename.substr(0, pos);
                    std::string s2 = filename.substr(pos);
                    fileCounter = (fileCounter + 1) % 10000;
                    std::ostringstream ostr;
                    ostr << s1 << std::setw(4) << std::setfill('0') << fileCounter << s2;
                    boost::filesystem::path path0(ostr.str(), boost::filesystem::native);
                    boost::filesystem::remove(path0);
                    boost::filesystem::copy_file(path, path0);
                    boost::filesystem::remove(path);
                }
                catch(const std::exception &e)
                {}
               
            }
            if (fout.get() == NULL)
            {
                fout.reset( new std::ofstream(filename.c_str()));
            }
        }
    }
   
} // namespace util
