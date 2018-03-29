#ifndef IMGV_PLUGIN_RASPICAM_HPP
#define IMGV_PLUGIN_RASPICAM_HPP

#include <imgv/imgv.hpp>
#include <imgv/plugin.hpp>
//#include "config.hpp"
//#include "raspicam.h"
#include "picam.hpp"

class pluginRaspicam : public plugin<blob> {

	int n; //compte le nombre d'outputs.

	public:
	pluginRaspicam();
	pluginRaspicam(message &m);
	~pluginRaspicam();

	std::string view;
	void init();
	void uninit();
	bool loop();

	private:
	int width;
	int height;
	bool initialized;
	bool paused; // si true, alors on saute les images qui arrivent
	int snapshot; // nb of snapshot to take (assume paused==true)
	bool formatIsGray; 
	picam::camera camera;
	bool decode(const osc::ReceivedMessage &m);
	double timestamp(void);
	void capture();

	// callback de la camera
	static void process(void *plugin,unsigned char *data,int len,int64_t pts);

};
#endif
