VERSION = 35.0.0

os2: {
DEFINES += 'VERSION_NUMBER=\'"35.0.0"\''
} else: {
  DEFINES += 'VERSION_NUMBER=\\\"$${VERSION}\\\"'
}


DEFINES += NOCRYPT \
          NOUNCRYPT



isEmpty(USE_ASPELL) {
         USE_ASPELL = true }

isEmpty(USE_HUNSPELL) {
         USE_HUNSPELL = true }

isEmpty(USE_PRINTER) {
         USE_PRINTER = true }



contains (USE_PRINTER,true)
{
  message ("Printer enabled")
  DEFINES += PRINTER_ENABLE
}


#contains(USE_CLANG,true){
#    message ("Clang enabled")
#    QMAKE_CC = clang
#    QMAKE_CXX = clang
#}



SOURCES += rvln.cpp \
    main.cpp \
    todo.cpp \
    textproc.cpp \
    libretta_calc.cpp \
    wavinfo.cpp \
    calendar.cpp \
    gui_utils.cpp \
    document.cpp \
    utils.cpp \
    spellchecker.cpp \
    fman.cpp \
    shortcuts.cpp \
    logmemo.cpp \
    img_viewer.cpp \
    tio.cpp \
    tzipper.cpp \
    ioapi.c \
    quazip.cpp \
    quazipfile.cpp \
    quazipnewinfo.cpp \
    unzip.c \
    zip.c

HEADERS += rvln.h \
    todo.h \
    document.h \
    utils.h \
    textproc.h \
    calendar.h \
    libretta_calc.h \
    spellchecker.h \
    fman.h \
    shortcuts.h \
    logmemo.h \
    img_viewer.h \
    gui_utils.h \
    wavinfo.h \
    zconf.h \
    zlib.h \
    tio.h \
    tzipper.h \
    ioapi.h \
    quazip.h \
    quazipfile.h \
    quazipfileinfo.h \
    quazipnewinfo.h \
    unzip.h \
    zip.h



TEMPLATE = app

CONFIG += warn_on \
    thread \
    qt \
    release \
    link_pkgconfig

QT += core
QT += gui


greaterThan(QT_MAJOR_VERSION, 4) {
       QT += widgets
       QT += printsupport
   } else {
#QT += blah blah blah
   }

TARGET = bin/tea
#target.path = /usr/local/bin

isEmpty(PREFIX) {
  PREFIX = /usr/local/bin
   }

target.path = $$PREFIX

INSTALLS += target
RESOURCES += rlvn.qrc
TRANSLATIONS = translations/tea_ru.ts \
               translations/tea_de.ts \
               translations/tea_fr.ts


DISTFILES += ChangeLog \
    COPYING \
    README \
    NEWS \
    NEWS-RU \
    AUTHORS \
    TODO \
    INSTALL \
    hls/* \
    icons/* \
    palettes/* \
    encsign/* \
    images/* \
    manuals/en.html \
    manuals/ru.html \
    text-data/* \
    translations/*

unix:  {
       	LIBS += -lz


contains(USE_ASPELL,true){
exists("/usr/include/aspell.h") {
    message ("ASpell enabled")
    LIBS += -laspell
    DEFINES += ASPELL_ENABLE
    DEFINES += SPELLCHECK_ENABLE
}
}


contains(USE_HUNSPELL,true){
exists("/usr/include/hunspell/hunspell.hxx") { 
    message ("hunspell enabled")
    #    LIBS += -lhunspell-1.2
    #    LIBS += $$system(pkg-config hunspell --libs)
        PKGCONFIG += hunspell
            DEFINES += HUNSPELL_ENABLE
                DEFINES += SPELLCHECK_ENABLE
                }
                }


}


win32: {

        LIBS += zlib1.dll

        contains(USE_ASPELL,true)
                 {
                  exists ("C:\\MinGw\\include\\aspell.h") 
                         { 
                          message ("ASpell enabled")
                          LIBS += -laspell-15
                          DEFINES += ASPELL_ENABLE
                          DEFINES += SPELLCHECK_ENABLE
                          }
                  }


        contains(USE_HUNSPELL,true)
                 {
                  exists ("C:\\MinGw\\include\\hunspell\\hunspell.hxx") 
                         {
                          message ("hunspell enabled")
                          LIBS += libhunspell.dll
                          DEFINES += HUNSPELL_ENABLE
                          DEFINES += SPELLCHECK_ENABLE
                         }
                }
       }



os2: {
contains(USE_ASPELL,true){
exists("q:/usr/include/aspell.h") {
     message ("ASpell enabled")
     LIBS += -laspell
     DEFINES += ASPELL_ENABLE
     DEFINES += SPELLCHECK_ENABLE
contains(USE_HUNSPELL,true)
}
}

{
exists("q:/usr/include/hunspell/hunspell.hxx") {
     message ("hunspell enabled")
     LIBS += -lhunspell-1.3
     PKGCONFIG += hunspell
     DEFINES += HUNSPELL_ENABLE
     DEFINES += SPELLCHECK_ENABLE
}
}

}

