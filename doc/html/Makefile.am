EXTRA_WEB_FILES = doxygen.css\
  doxygen.png

HTML_FILES = annotated.html\
  files.html\
  functions.html\
  globals.html\
  group__activation.html\
  group__callbacks.html\
  group__currents.html\
  group__debugerr.html\
  group__enum.html\
  group__lookup.html\
  group__props.html\
  group__settings.html\
  group__wininfo.html\
  group__xkbevents.html\
  group__xkbgroup.html\
  group__xkbinfo.html\
  group__xklconfig.html\
  group__xklconfiginitterm.html\
  group__xklinitterm.html\
  index.html\
  modules.html\
  struct__XklConfigItem.html\
  struct__XklConfigItem-members.html\
  struct__XklConfigRec.html\
  struct__XklConfigRec-members.html\
  structXklState.html\
  structXklState-members.html\
  xklavier_8h.html\
  xklavier_8h-source.html\
  xklavier__config_8h.html\
  xklavier__config_8h-source.html

DOXYGEN_OUT_FILES = $(EXTRA_WEB_FILES) $(HTML_FILES)

html_to = $(datadir)/doc/$(PACKAGE)-$(VERSION)/html

html_files = $(DOXYGEN_OUT_FILES)

install:
	@$(NORMAL_INSTALL)
	if test "x$(DO_DOXYGEN)" = "xyes"; then \
	  $(mkinstalldirs) $(DESTDIR)$(html_to); \
	  for p in $(html_files); do \
	    $(INSTALL_DATA) $$p $(DESTDIR)$(html_to)/$$p; \
	  done; \
	fi

uninstall:
	@$(NORMAL_UNINSTALL)
	for p in $(html_files); do \
	  rm -f $(DESTDIR)$(html_to)/$$p; \
	done

MAINTAINERCLEANFILES = $(DOXYGEN_OUT_FILES)

