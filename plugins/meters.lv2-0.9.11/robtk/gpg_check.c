#ifdef WITH_SIGNATURE

/* 'self' struct needs
 * bool  gpg_verified;
 * char  gpg_data[128];
 */

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE -1
#endif

{
	self->gpg_verified = FALSE;
	gp3_initialize ();
	load_master_key (); // in header WITH_SIGNATURE
	gp3_loglevel (GP3L_SILENT);
	int rc = -1;
	char signature_file0[1024] = "";
	char signature_file1[1024] = "";
	char signature_file2[1024] = "";
	char signature_file3[1024] = "";
#ifdef SIGNOVERSION
	memset(self->gpg_data, 0, sizeof (self->gpg_data));
#else
	strcpy(self->gpg_data, "v" VERSION);
#endif
#ifdef _WIN32
	ExpandEnvironmentStrings("%localappdata%\\"SIGFILE, signature_file0, 1024);
	ExpandEnvironmentStrings("%localappdata%\\x42_license.txt", signature_file2, 1024);

	const char * homedrive = getenv("HOMEDRIVE");
	const char * homepath = getenv("HOMEPATH");

	if (homedrive && homepath && (strlen(homedrive) + strlen(homepath) + strlen(SIGFILE) + 3) < 1024) {
		sprintf(signature_file1, "%s%s\\%s", homedrive, homepath, SIGFILE);
	}
	if (homedrive && homepath && (strlen(homedrive) + strlen(homepath) + strlen(SIGFILE) + 17) < 1024) {
		sprintf(signature_file3, "%s%s\\x42_license.txt", homedrive, homepath);
	}
#else
	const char * home = getenv("HOME");
	if (home && (strlen(home) + strlen(SIGFILE) + 3) < 1024) {
		sprintf(signature_file0, "%s/%s", home, SIGFILE);
	}
	if (home && (strlen(home) + strlen(SIGFILE) + 3) < 1024) {
		sprintf(signature_file1, "%s/.%s", home, SIGFILE);
	}
	if (home && (strlen(home) + 18) < 1024) {
		sprintf(signature_file2, "%s/x42_license.txt", home);
	}
	if (home && (strlen(home) + 18) < 1024) {
		sprintf(signature_file3, "%s/.x42_license.txt", home);
	}
#endif
	if (testfile(signature_file0)) {
		rc = gp3_checksigfile (signature_file0);
	} else if (testfile(signature_file1)) {
		rc = gp3_checksigfile (signature_file1);
	} else if (testfile(signature_file2)) {
		rc = gp3_checksigfile (signature_file2);
	} else if (testfile(signature_file3)) {
		rc = gp3_checksigfile (signature_file3);
	} else {
#if 0
		fprintf(stderr, " *** no signature file found\n");
#endif
	}
	if (rc == 0) {
		char data[8192];
		char *tmp=NULL;
		uint32_t len = gp3_get_text(data, sizeof(data));
		if (len == sizeof(data)) data[sizeof(data)-1] = '\0';
		else data[len] = '\0';
#if 0
		fprintf(stderr, " *** signature:\n");
		if (len > 0) fputs(data, stderr);
#endif
		if ((tmp = strchr(data, '\n'))) *tmp = 0;
		self->gpg_data[sizeof(self->gpg_data) - 1] = 0;
		if (tmp++ && *tmp) {
			if ((tmp = strstr(tmp, RTK_URI))) {
				char *t1, *t2;
				self->gpg_verified = TRUE;
				t1 = tmp + strlen(RTK_URI);
				t2 = strchr(t1, '\n');
				if (t2) { *t2 = 0; }
				if (strlen(t1) > 0 && strncmp(t1, VERSION, strlen(t1))) {
					self->gpg_verified = FALSE;
				}
			}
		}
		if (!self->gpg_verified) {
#if 0
			fprintf(stderr, " *** signature is not valid for this version/bundle.\n");
#endif
		} else {
#ifndef SIGNOVERSION
			strncat(self->gpg_data, " ", sizeof(self->gpg_data) - strlen(self->gpg_data));
#endif
			strncat(self->gpg_data, data, sizeof(self->gpg_data) - strlen(self->gpg_data));
		}
	}
	gp3_cleanup ();
}
#endif
