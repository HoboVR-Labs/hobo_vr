#if defined(LINUX)
	#define PLUGIN_DLL_EXPORT extern "C" __attribute__((visibility("default")))
	#define PLUGIN_DLL_IMPORT extern "C"
#elif defined(WIN)
	#define PLUGIN_DLL_EXPORT extern "C" __declspec(dllexport)
	#define PLUGIN_DLL_IMPORT extern "C" __declspec(dllimport)
#else
	#error "Unsupported Platform."
#endif

#include "gaze_master_plugin_interface.h"

#include <fstream>

class GazeLogger: public gaze::PluginBase {
public:
	GazeLogger() = default;
	virtual ~GazeLogger() = default;

	virtual bool Activate() {
		m_file = std::move(std::ofstream("/tmp/GazeLogger_out.log", std::ios::binary));
		h = 0;
		return !m_file.bad();
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
		return "failed to open file";
	}

private:
	std::ofstream m_file;
	int h = 0;
};

PLUGIN_DLL_EXPORT gaze::PluginBase* gazePluginFactory() {
	return new GazeLogger;
}