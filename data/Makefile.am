NULL=

SUBDIRS = 

@INTLTOOL_DESKTOP_RULE@
desktopdir=$(datadir)/applications
desktop_in_files = nwam-manager-properties.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

autosessiondir      = $(sysconfdir)/xdg/autostart
autosession_in_files = nwam-manager.desktop.in
autosession_DATA    = $(autosession_in_files:.desktop.in=.desktop)

@INTLTOOL_SCHEMAS_RULE@
schemasdir       = $(GCONF_SCHEMA_FILE_DIR)
schemas_in_files = nwam-manager.schemas.in
schemas_DATA     = $(schemas_in_files:.schemas.in=.schemas)

if GCONF_SCHEMAS_INSTALL
install-data-local:
	if test -z "$(DESTDIR)" ; then \
		for p in $(schemas_DATA) ; do \
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(top_builddir)/daemon/$$p ; \
		done ; \
	fi
else
install-data-local:
endif

DISTCLEANFILES = \
	$(autosession_DATA)	\
        $(desktop_DATA) \
        $(schemas_DATA) \
        $(NULL)

dist-hook:
	cd $(distdir) ; rm -f $(DISTCLEANFILES)

EXTRA_DIST = \
	$(schemas_in_files)	\
        $(desktop_in_files)	\
	$(autosession_in_files)	\
	$(NULL)
