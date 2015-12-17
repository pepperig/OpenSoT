/*
 * Copyright (C) 2014 Walkman
 * Author: Alessio Rocchi, Enrico Mingo
 * email:  alessio.rocchi@iit.it, enrico.mingo@iit.it
 * Permission is granted to copy, distribute, and/or modify this program
 * under the terms of the GNU Lesser General Public License, version 2 or any
 * later version published by the Free Software Foundation.
 *
 * A copy of the license can be found at
 * https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
 * Public License for more details
*/

#ifndef __PLOTTER_H__
#define __PLOTTER_H__

#include <OpenSoT/Constraint.h>
#include <OpenSoT/Task.h>
#include <OpenSoT/utils/logger/flushers/all.h>
#include <OpenSoT/utils/logger/flushers/ConstraintFlusher.h>
#include <OpenSoT/utils/logger/flushers/DataFlusher.h>
#include <OpenSoT/utils/logger/flushers/FakeFlusher.h>
#include <OpenSoT/utils/logger/flushers/TaskFlusher.h>
#include <map>
#include <fstream>
#include <list>
#include <sstream>
#include <string>
#include <utility>

namespace OpenSoT
{
    // forward declaration of L
    class L;

    namespace plotters
    {
        /**
         * @brief Plottable is a pair of a pointer to Flusher and an Indices set.
         * If the pointer to flusher is NULL, the indices will be considered as global.
         * This will happen e.g. when the indices make reference to a time series, or to the solution vector,
         * or to the norm vector.
         */
        typedef std::pair<OpenSoT::flushers::Flusher*, OpenSoT::Indices> Plottable;

        /**
         * @brief The Plotter class takes care of generating
         */
        class Plotter
        {
        protected:
            /**
             * @brief _logger is a reference to a logger.
             * It is used to transform local Plottable indices in global indices,
             * which depend on the order in which the plottables are flushed.
             * The _logger also defines which is the plotting format
             */
            L* _logger;

            /**
             * @brief logCommands a script with commands to generate the plot.
             * The commands will depend on the format of the logger.
             */
            std::stringstream _commands;

            int _n_fig;

            std::list<flushers::Flusher::Ptr> _fakeFlushers;

            std::list<unsigned int> getGlobalIndicesList(std::list<OpenSoT::plotters::Plottable> data);

            std::string getIndicesString(std::list<unsigned int> indices);

        public:
            typedef boost::shared_ptr<Plotter> Ptr;

            Plotter(L* logger);

            void subplot(const unsigned int nRows,
                         const unsigned int nCols,
                         const unsigned int nSubPlot);

            void figure(const unsigned int width_in,
                        const unsigned int height_in,
                        const std::string title);

            void xlabel(const std::string& label);

            void ylabel(const std::string& label);

            Plottable norm(std::list<Plottable> data);

            /**
             * @brief legend builds a legend from a list of labels
             * @param labels a list of string, one for each plot line
             */
            void legend(const std::list<std::string> labels);

            /**
             * @brief autoLegend build a legend from the flusher data description
             */
            void autoLegend(std::list<Plottable> data);

            /**
             * @brief autoLegend build a legend from the flusher data description
             */
            void autoLegend(Plottable data);

            /**
             * @brief autoLegend builds a list of labels from the flusher data description
             */
            std::list<std::string> autoGenerateLegend(std::list<Plottable> data);

            /**
             * @brief plot_t plots data against time
             * @param data a list of plottables
             */
            void plot_t(std::list<Plottable> data);

            /**
             * @brief plot_t plots data against time
             * @param data a list of plottables
             */
            void plot_t(Plottable data);

            /**
             * @brief savefig saves the last defined figure
             */
            void savefig();

            /**
             * @brief show displays plots on screen
             */
            void show();

            void title(const std::string& title);

            /*bool plot(const Plottable& index, std::list<Plottable> data);*/

            std::string getCommands();
        };

        std::list<Plottable> operator+(const Plottable& p1, const Plottable& p2);
        std::list<Plottable> operator+(const Plottable& p1, std::list<Plottable>& pl2);
        std::list<Plottable> operator+(std::list<Plottable>& pl1, const Plottable& p2);
        std::list<Plottable> operator+(std::list<Plottable>& pl1, std::list<Plottable>& pl2);
    }
}

#endif
