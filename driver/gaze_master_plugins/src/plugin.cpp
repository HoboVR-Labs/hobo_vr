#ifdef WIN
#include <cstring>
#include <cstdarg>
#include <string>
#include <iostream>
    // #define _snprintf _snprintf_s
    // #define snprintf _snprintf
    // #undef snprintf
    namespace std {
        inline int snprintf (char *s, size_t maxlen, const char *format, ...)
        {
          va_list arg;
          int done;
          va_start (arg, format);
          done = _snprintf_s(s, maxlen, maxlen, format, arg, 0);
          va_end (arg);
          return done;
        }
    }
#endif
#include <boost/dll.hpp>

#if !defined(WIN) && !defined(LINUX)
#error "incorrect setup"
#endif

#include "gaze_master_plugin_interface.h"

#include <fstream>

namespace glog {

class GazeLogger: public gaze::PluginBase {
public:
	GazeLogger() = default;
	virtual ~GazeLogger() = default;

	virtual bool Activate(const gaze::PluginVersion_t version) {
		if (version.major != gaze::g_plugin_interface_version_major &&
			version.minor != gaze::g_plugin_interface_version_minor &&
			version.hotfix != gaze::g_plugin_interface_version_hotfix)
		{
			m_error_msg = "plugin interface version miss match";
			return false; // interface version incompatibility is a no no
		}

		#ifdef WIN
		m_file = std::move(std::ofstream(".\\GazeLogger_out.log", std::ios::binary));
		#else
		m_file = std::move(std::ofstream("/tmp/GazeLogger_out.log", std::ios::binary));
		#endif
		
		h = 0;
		return !m_file.bad() && m_file.is_open();
	}

	virtual void ProcessData(const HoboVR_GazeState_t& data) {
		if (h % 200 == 0 && m_file.is_open()) {
			m_file << "gaze data: " << data.status << ", "
				<< data.age_seconds << ", ("
				<< data.pupil_position_r[0] << "," << data.pupil_position_r[1] << "), "
				<< data.pupil_position_l[0] << "," << data.pupil_position_l[1] << "), "
				<< data.pupil_dilation_r << ", " << data.pupil_dilation_l << ", "
				<< data.eye_close_r << ", " << data.eye_close_l << ";\n";
			m_file.flush();
			h = 0;
		}
		h++;
	}

	virtual std::string GetNameAndVersion() {
		return "GazeLogger_v0.0.1";
	}

	virtual std::string GetLastError() {
		return m_error_msg;
	}


private:
	std::ofstream m_file;
	int h = 0;
	std::string m_error_msg = "failed to open file";
};

gaze::PluginBase* factory() {
	auto m_file = std::ofstream("/tmp/GazeLogger_out.alive.log", std::ios::binary);
	m_file << "yes";
	return reinterpret_cast<gaze::PluginBase*>(new GazeLogger);
}

} // namespace glog

extern "C" BOOST_SYMBOL_EXPORT gaze::PluginBase* gazePluginFactory() {
	return glog::factory();
}