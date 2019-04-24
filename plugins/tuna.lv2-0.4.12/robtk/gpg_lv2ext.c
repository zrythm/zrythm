struct _gpginfo {
	int gpg_verified;
	int gpg_checked;
	char gpg_data[128];
};

static struct _gpginfo gpginfo = {0, 0, };

// return -1 if no license is needed, 1 if licensed, 0 if not licensed
static int
is_licensed (LV2_Handle instance)
{
	if (!gpginfo.gpg_checked) {
		gpginfo.gpg_checked = 1;
#define self (&gpginfo)
#define SIGNOVERSION
#include "gpg_check.c"
#undef self
	}
	return gpginfo.gpg_verified ? 1 : 0;
}

static char*
licensee (LV2_Handle instance)
{
	if (gpginfo.gpg_verified) {
		if (strlen (gpginfo.gpg_data) > 13 && !strncmp(gpginfo.gpg_data, "Licensed to ", 12)) {
			return strdup (&gpginfo.gpg_data[12]);
		} else {
			return strdup (gpginfo.gpg_data);
		}
	}
	return NULL;
}

static const char*
product_uri (LV2_Handle instance)
{
	  return RTK_URI;
}

static const char*
product_name (LV2_Handle instance)
{
	  return license_infos.name;
}

static const char*
store_url (LV2_Handle instance)
{
	  return license_infos.store;
}

#define LV2_LICENSE_EXT_C \
	static const LV2_License_Interface lif = { &is_licensed, &licensee, &product_uri, &product_name, &store_url }; \
	if (!strcmp(uri, LV2_PLUGINLICENSE__interface)) { return &lif; }
