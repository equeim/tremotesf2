#ifndef TREMOTESF_COMMANDLINEPARSER_H
#define TREMOTESF_COMMANDLINEPARSER_H

#include <QStringList>

namespace tremotesf
{
    struct CommandLineArgs
    {
        QStringList files;
        QStringList urls;
        bool minimized;

        bool exit;
    };

    CommandLineArgs parseCommandLine(int& argc, char**& argv);
}

#endif // TREMOTESF_COMMANDLINEPARSER_H
