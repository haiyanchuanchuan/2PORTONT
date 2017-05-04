#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include "web_voip.h"
#include "sip_version.h"
#if CONFIG_RTK_VOIP_PACKAGE_867X
#include <fcntl.h>
#endif /* CONFIG_RTK_VOIP_PACKAGE_867X */
#if CONFIG_RTK_VOIP_PACKAGE_8186
#include "apmib.h"
#endif

#define FIFO_SOLAR "/var/run/solar_control.fifo"

char dspVersion[30]={0};
char *sipVersion = SIP_VERSION;

#ifdef CONFIG_RTK_VOIP_PACKAGE_867X
//shlee, fix merging problem
char *CURRENT_SETTING_VER = SIP_VERSION;
#endif

static void get_voip_version()
{
	FILE *fp;

	fp = fopen("/proc/voip/version", "r");

	if (fp)
	{
		fgets(dspVersion, sizeof(dspVersion)-1, fp);
		fclose(fp);
    }
}

static void web_voip_abort(int sig)
{
	printf("webs: web_voip_abort\n");
	voip_flash_server_stop();
	exit(0);
}

/* SD6, bohungwu, enable RTK VoIP demo page on E8C by environment variable */
char *rcm_dbg_on;

int web_voip_init()
{
	FILE *fh;
	char buf[MAX_VOIP_PORTS * MAX_PROXY];

	/* To show the register status on Web page. */
	fh = fopen(_PATH_TMP_STATUS, "w");
	if (!fh) {
		printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
		printf("\nerrno=%d\n", errno);
	}
	else {
		memset(buf, (int) '0', sizeof(buf));
		fwrite(buf, sizeof(buf), 1, fh);
		fclose(fh);
	}

	get_voip_version();
#if defined(CONFIG_APP_BOA)
// TODO
#else
	//[SD6, bohungwu, e8c web
	boaASPDefine("voip_e8c_get", asp_voip_e8c_get);
	boaFormDefine("voip_e8c_set", asp_voip_e8c_set);
	boaFormDefine("voip2_e8c_set", asp_voip2_e8c_set);
	//]
	// general page
	boaASPDefine("voip_general_get", asp_voip_GeneralGet);
	boaFormDefine("voip_general_set", asp_voip_GeneralSet);
	// dialplan page
	boaASPDefine("voip_dialplan_get", asp_voip_DialPlanGet);
	boaFormDefine("voip_dialplan_set", asp_voip_DialPlanSet);
	// tone page
	boaASPDefine("voip_tone_get", asp_voip_ToneGet);
	boaFormDefine("voip_tone_set", asp_voip_ToneSet);
	// ring page
	boaASPDefine("voip_ring_get", asp_voip_RingGet);
	boaFormDefine("voip_ring_set", asp_voip_RingSet);
	// other page
	boaASPDefine("voip_other_get", asp_voip_OtherGet);
	boaFormDefine("voip_other_set", asp_voip_OtherSet);
	// config page
	boaASPDefine("voip_config_get", asp_voip_ConfigGet);
	boaFormDefine("voip_config_set", asp_voip_ConfigSet);
	// fw update daemon
	boaASPDefine("voip_fwupdate_get", asp_voip_FwupdateGet);
	boaFormDefine("voip_fw_set", asp_voip_FwSet);
	// net page
	boaFormDefine("voip_net_set", asp_voip_NetSet);
	boaASPDefine("voip_net_get", asp_voip_NetGet);

	//call history page
	boaFormDefine("voip_log_set", asp_voip_LogSet);
	boaASPDefine("voip_log_get", asp_voip_LogGet);
	
	// tr069 oage
#ifdef CONFIG_RTK_VOIP_IVR
	boaFormDefine("voip_ivrreq_set", asp_voip_IvrReqSet);
#endif
	// sip tls page
#ifdef CONFIG_RTK_VOIP_SIP_TLS
	boaFormDefine("voip_TLSCertUpload", asp_voip_TLSCertUpload);
	boaASPDefine("voip_TLSGetCertInfo", asp_voip_TLSGetCertInfo);
#endif
#endif

	signal(SIGINT, web_voip_abort);
	signal(SIGTERM, web_voip_abort);

#if CONFIG_RTK_VOIP_PACKAGE_865X
/*If 865x, do voip_flash_server_start() at Init voip in boa/src/rtl/proc/proc_general.c */
#else
	if (voip_flash_server_start() != 0)
	{
		fprintf(stderr, "web_voip_init: voip_flash_server_start failed\n");
		return -1;
	}
#endif
	web_voip_saveConfig();

	/* SD6, bohungwu, enable RTK VoIP demo page on E8C by environment variable */
	rcm_dbg_on = getenv("RCM_DBG_ON");
	return 0;
}

int web_restart_solar()
{
	int h_pipe, res, refresh;
	FILE *fh;
	char buf[MAX_VOIP_PORTS * MAX_PROXY];

	refresh = voip_flash_server_update();
	if (refresh == -1)
		return -1;

	// restart solar, it will reload flash settings
	if (access(FIFO_SOLAR, F_OK) != 0)
	{
		fprintf(stderr, "access %s failed\n", FIFO_SOLAR);
		return -1;
	}

	h_pipe = open(FIFO_SOLAR, O_WRONLY | O_NONBLOCK);
	if (h_pipe == -1)
	{
		fprintf(stderr, "open %s failed\n", FIFO_SOLAR);
		return -1;
	}

#if 0
	if (refresh)
	{
		fprintf(stderr, "web_restart_solar: refresh...\n");
		res = write(h_pipe, "r\n", 2);		// refresh solar
	}
	else
	{
		fprintf(stderr, "web_restart_solar: restart...\n");
		res = write(h_pipe, "x\n", 2);		// restart solar
	}
#else
	// always restatr solar for networking:
	// restart solar for multiple VLANs on WAN port.
	fprintf(stderr, "web_restart_solar: restart...\n");
	res = write(h_pipe, "x\n", 2);		// restart solar
#endif

	if (res == -1)
	{
		fprintf(stderr, "write %s failed\n", FIFO_SOLAR);
		close(h_pipe);
		return -1;
	}

	close(h_pipe);

	/* To show the register status on Web page. */
	fh = fopen(_PATH_TMP_STATUS, "w");
	if (!fh) {
		printf("Warning: cannot open %s. Limited output.\n", _PATH_TMP_STATUS);
		printf("\nerrno=%d\n", errno);
	}
	else {
		printf("Reset register status...\n");
		memset(buf, (int) '0', sizeof(buf));
		fwrite(buf, sizeof(buf), 1, fh);
		fclose(fh);
	}

	return 0;
}

void run_init_script_announcement( char *arg )
{
	/* web is going to execute 'init.sh' */
	voip_flash_server_update();
}

#if CONFIG_RTK_VOIP_PACKAGE_865X

int asp_voip_getInfo(request * wp, int argc, char **argv)
{
	char	*name;

   	if (argc != 1) {
   		boaWrite(wp, "Insufficient args %d\n", argc);
   		return -1;
   	}
	name = argv[0];

	if (strcmp(name, "voip_status") == 0)
	{
		boaWrite(wp,
			"<tr>" \
			"<td width=100%% colspan=2 bgcolor=#008000><font color=#FFFFFF size=2><b>VoIP</b></font></td>" \
			"</tr>" \
			"<tr bgcolor=#DDDDDD>" \
			"<td width=40%%><font size=2><b>Version</b></td>" \
			"<td width=60%%><font size=2>%s</td>" \
			"</tr>", sipVersion);
		return 0;
	}
	else if (strcmp(name, "voip_menu") == 0)
	{
		int i;
#ifdef CONFIG_RTL865XB
		boaWrite(wp,
			"<tr><td><span id=spanHead8 onclick=\"menu8()\">" \
			"<a href=#><B>VoIP</a><BR>" \
			"</span>" \
			"<div id=spanMenu8 STYLE=\"display:none\">" \
			"<table CELLSPACING=0 CELLPADDING=0>");
#else
		boaWrite(wp,
			"<tr><td><span id=spanHead_voip onclick=\"menu_voip()\">" \
			"<a href=#><B>VoIP</a><BR>" \
			"</span>" \
			"<div id=spanMenu_voip STYLE=\"display:none\">" \
			"<table CELLSPACING=0 CELLPADDING=0>");
#endif
		for (i=0; i<g_VoIP_Ports; i++)
		{
			if (g_VoIP_Ports == 1)
				boaWrite(wp,
					"<tr><td width=10></td><td><a href=voip_general.asp?port=0 target=show_area><span style=\"font-size: 12pt;\">General</span></a></td></tr>");
			else if (RTK_VOIP_IS_DECT_CH( i, g_VoIP_Feature ))
				boaWrite(wp,
					"<tr><td width=10></td><td><a href=voip_general.asp?port=%d target=show_area><span style=\"font-size: 12pt;\">DECT %d</span></a></td></tr>", i, i - RTK_VOIP_DECT_CH_OFFSET( g_VoIP_Feature ) );
			else if (RTK_VOIP_IS_SLIC_CH( i, g_VoIP_Feature ))
				boaWrite(wp,
					"<tr><td width=10></td><td><a href=voip_general.asp?port=%d target=show_area><span style=\"font-size: 12pt;\">FXS %d</span></a></td></tr>", i, i - RTK_VOIP_SLIC_CH_OFFSET( g_VoIP_Feature ) );
			else
				boaWrite(wp,
					"<tr><td width=10></td><td><a href=voip_general.asp?port=1 target=show_area><span style=\"font-size: 12pt;\">FXO %d</span></a></td></tr>", i, i - RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature ) );
		}

#ifdef CONFIG_RTK_VOIP_IP_PHONE
		boaWrite(wp,
			"<tr><td width=10></td><td><a href=voip_tone.asp target=show_area><span style=\"font-size: 12pt;\">Tone</span></a></td></tr>" \
			"<tr><td width=10></td><td><a href=voip_config.asp target=show_area><span style=\"font-size: 12pt;\">Config</span></a></td></tr>");
#else
		boaWrite(wp,
			"<tr><td width=10></td><td><a href=voip_tone.asp target=show_area><span style=\"font-size: 12pt;\">Tone</span></a></td></tr>" \
			"<tr><td width=10></td><td><a href=voip_ring.asp target=show_area><span style=\"font-size: 12pt;\">Ring</span></a></td></tr>" \
			"<tr><td width=10></td><td><a href=voip_other.asp target=show_area><span style=\"font-size: 12pt;\">Other</span></a></td></tr>" \
			"<tr><td width=10></td><td><a href=voip_config.asp target=show_area><span style=\"font-size: 12pt;\">Config</span></a></td></tr>");
#endif
		boaWrite(wp,
			"</table></div>" \
			"</td></tr>");
		return 0;
	}

	return -1;
}

#else

#ifdef CONFIG_APP_BOA
int asp_voip_getInfo(request * wp, int argc, char **argv)
#else
int asp_voip_getInfo(int eid, request * wp, int argc, char **argv)
#endif
{
	char	*name;
	int i;
/*+++added by Jack for auto configuration+++*/
	voipCfgParam_t *pVoIPCfg;

	if (web_flash_get(&pVoIPCfg) != 0)
		return -1;
/*---end---*/

#ifdef CONFIG_APP_BOA
   	if (argc != 1) {
   		boaWrite(wp, "Insufficient args %d\n", argc);
   		return -1;
   	}
	name = argv[0];
#else
   	if (boaArgs(argc, argv, "%s", &name) < 1) {
   		boaError(wp, 400, "Insufficient args\n");
   		return -1;
   	}
#endif


#ifndef CONFIG_RTK_VOIP_MULTI_WAN_CTYPE

	if(strcmp(name,"voip_wan_intf")==0){


	//	printf(" pVoIPCfg->voip_interface is %s\n", pVoIPCfg->voip_interface);
	//	return 0;

	
		if(strlen(pVoIPCfg->voip_interface)==0){
			//pVoIPCfg->voip_interface is null , any wan 
			//printf("voip_interface is null \n");
			return boaWrite(wp,
				"<script language=\"javascript\">" \
					"document.net_form.voip_itf.selectedIndex = 0;"\
				"</script>");

			
		}else if(strcmp(pVoIPCfg->voip_interface,"br0")==0){
			//printf("22 voip_interface is br0 \n");

			return boaWrite(wp,
				"<script language=\"javascript\">" \
					"document.net_form.voip_itf.selectedIndex = 1;"\
				"</script>");
			
		}else{

			return boaWrite(wp,
			"<script language=\"javascript\">" \
			"document.net_form.voip_itf.selectedIndex = -1;"\
			"for( i = 0; i < document.net_form.voip_itf.options.length; i++ )"\
			"{"\
				"if( \"%s\" == document.net_form.voip_itf.options[i].text )"\
				"document.net_form.voip_itf.selectedIndex = i;"\		
			"}"\
			"</script>",pVoIPCfg->voip_interface);
		}

	}
#endif	
	if (strcmp(name, "voip_status") == 0)
	{
		return boaWrite(wp,
			"<tr>" \
			"<td width=100%% colspan=2 bgcolor=#008000><font color=#FFFFFF size=2><b>VoIP</b></font></td>" \
			"</tr>" \
			"<tr bgcolor=#DDDDDD>" \
			"<td width=40%%><font size=2><b>VoIP Version</b></td>" \
			"<td width=60%%><font size=2>%s-%s</td>" \
/*+++added by Jack for auto configuration+++*/
			"<tr bgcolor=#DDDDDD>" \
			"<td width=40%%><font size=2><b>Flash Version</b></td>" \
			"<td width=60%%><font size=2>%d.%d</td>" \
			"</tr>" \
			"<tr bgcolor=#DDDDDD>" \
			"<td width=40%%><font size=2><b>Autoconfig Version</b></td>" \
			"<td width=60%%><font size=2>%d</td>" \
			"</tr>" \
			"<tr bgcolor=#DDDDDD>" \
			"<td width=40%%><font size=2><b>Firmware upgrade Version</b></td>" \
			"<td width=60%%><font size=2>%s</td>" \
/*---end---*/
			"</tr>",dspVersion ,sipVersion, CURRENT_SETTING_VER, VOIP_FLASH_VER,
					pVoIPCfg->auto_cfg_ver, pVoIPCfg->fw_update_fw_version);
	}
	else if (strcmp(name, "voip_menu") == 0)
	{
		boaWrite(wp,"document.write('" \
				"<tr><td><b>VoIP Settings</b></td></tr>");

		for (i=0; i<g_VoIP_Ports; i++)
		{
			if (g_VoIP_Ports == 1)
				boaWrite(wp,
					"<tr><td><a href=voip_general.asp?port=0 target=view>General</a></td></tr>");
			else if (RTK_VOIP_IS_DECT_CH( i, g_VoIP_Feature ))
				boaWrite(wp,
					"<tr><td><a href=voip_general.asp?port=%d target=view>DECT %d</a></td></tr>",
					i, i - RTK_VOIP_DECT_CH_OFFSET( g_VoIP_Feature ) );
			else if (RTK_VOIP_IS_SLIC_CH( i, g_VoIP_Feature ))
				boaWrite(wp,
					"<tr><td><a href=voip_general.asp?port=%d target=view>FXS %d</a></td></tr>",
					i, i - RTK_VOIP_SLIC_CH_OFFSET( g_VoIP_Feature ) );
			else
				boaWrite(wp,
					"<tr><td><a href=voip_general.asp?port=%d target=view>FXO %d</a></td></tr>",
					i, i - RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature ) );
		}

#ifdef CONFIG_RTK_VOIP_IP_PHONE
		return boaWrite(wp,
				"<tr><td><a href=voip_tone.asp target=view>Tone</a></td></tr>" \
				"<tr><td><a href=voip_config.asp target=view>Config</a></td></tr>" \
				"')");
#else
		return boaWrite(wp,
				"<tr><td><a href=voip_tone.asp target=view>Tone</a></td></tr>" \
				"<tr><td><a href=voip_ring.asp target=view>Ring</a></td></tr>" \
				"<tr><td><a href=voip_other.asp target=view>Other</a></td></tr>" \
				"<tr><td><a href=voip_config.asp target=view>Config</a></td></tr>" \
				"')");
#endif
	}
	else if (strcmp(name, "voip_tree_menu") == 0)
	{
		boaWrite(wp,
				"menu.addItem('VoIP Settings');" \
				"voip = new MTMenu();");

		for (i=0; i<g_VoIP_Ports; i++)
		{
			if (g_VoIP_Ports == 1)
				boaWrite(wp,
					"voip.addItem('General', 'voip_general.asp?port=0', '', 'Setup General settings');");
			else if (RTK_VOIP_IS_DECT_CH( i, g_VoIP_Feature ))
				boaWrite(wp,
					"voip.addItem('DECT %d', 'voip_general.asp?port=%d', '', 'Setup DECT%d settings');",
					i - RTK_VOIP_DECT_CH_OFFSET( g_VoIP_Feature ), i, i);
			else if (RTK_VOIP_IS_SLIC_CH( i, g_VoIP_Feature ))
				boaWrite(wp,
					"voip.addItem('FXS %d', 'voip_general.asp?port=%d', '', 'Setup FXS%d settings');",
					i - RTK_VOIP_SLIC_CH_OFFSET( g_VoIP_Feature ), i, i);
			else
				boaWrite(wp,
					"voip.addItem('FXO %d', 'voip_general.asp?port=%d', '', 'Setup FXO%d settings');",
					i - RTK_VOIP_DAA_CH_OFFSET( g_VoIP_Feature ), i, i);
		}

#ifdef CONFIG_RTK_VOIP_IP_PHONE
		return boaWrite(wp,
			"voip.addItem('Tone', 'voip_tone.asp', '', 'Setup Tone settings');" \
			"voip.addItem('Config', 'voip_config.asp', '', 'Config settings');" \
			"voip.addItem('Network', 'voip_network.asp', '', 'Setup Network Settings');" \
			"menu.makeLastSubmenu(voip);");
#else
		return boaWrite(wp,
			"voip.addItem('Tone', 'voip_tone.asp', '', 'Setup Tone settings');" \
			"voip.addItem('Ring', 'voip_ring.asp', '', 'Setup Ring settings');" \
			"voip.addItem('Other', 'voip_other.asp', '', 'Setup Other settings');" \
			"voip.addItem('Config', 'voip_config.asp', '', 'Config settings');" \
			"voip.addItem('Network', 'voip_network.asp', '', 'Setup Network Settings');" \
			"menu.makeLastSubmenu(voip);");
#endif
	}

	return -1;
}

#endif

// flash api in WEB
int web_flash_get(voipCfgParam_t **cfg)
{
	return voip_flash_get(cfg);
}

int web_flash_set(voipCfgParam_t *cfg)
{
	int ret;

	ret = voip_flash_set(cfg);
	if (ret == 0)
		web_voip_saveConfig();

	return ret;
}

// save voip config to config_voip.dat
int web_voip_saveConfig()
{
#if 0 //ericchung: new trunk not need export voip mib to file 
	voipCfgAll_t cfg_all;
	voipCfgParam_t *pVoIPCfg;
#ifndef CONFIG_RTK_VOIP_PACKAGE_867X
	extern int save_cs_to_file();
#endif

#ifndef CONFIG_RTK_VOIP_PACKAGE_867X
	save_cs_to_file();
#endif

	if (web_flash_get(&pVoIPCfg) != 0)
		return -1;

	memset(&cfg_all, 0, sizeof(cfg_all));
	memcpy(&cfg_all.current_setting, pVoIPCfg, sizeof(voipCfgParam_t));
	cfg_all.mode |= VOIP_CURRENT_SETTING;
	return flash_voip_export_to_file(&cfg_all, VOIP_CONFIG_PATH, 1);
#endif	
}
