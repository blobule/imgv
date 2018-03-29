#ifndef IMGV_PLUGIN_SYSLOG_HPP
#define IMGV_PLUGIN_SYSLOG_HPP

#include <imgv/imgv.hpp>

#include <imgv/plugin.hpp>


//
// syslog plugin
//
// [in] (0) : queue d'entree des messages
//
// S'attend a des messages du genre:
// /info <plugin name> <message>
// /warn <plugin name> <message>
// /err <plugin name> <message>
// -> affiche ces contenus.
//
// une commande;
// /filter/reset <true|false> -> filtre tous les plugins (reset les filtres)
//    -> default a depart: all on
// /filter <plugin name> <bool=true/false>  pour voir ou pas des message (pas reset)
//


class pluginSyslog : public plugin<blob> {
    public:
    private:
	// filter stuff....
    public:
	pluginSyslog();
	~pluginSyslog();
	void init();
	void uninit();
	bool loop();

    private:
	bool decode(const osc::ReceivedMessage &m);
};


#endif

