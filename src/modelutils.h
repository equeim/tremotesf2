 /*
 * Tremotesf
 * Copyright (C) 2015-2020 Alexey Rochev <equeim@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TREMOTESF_MODELUTILS_H
#define TREMOTESF_MODELUTILS_H

#include <QAbstractItemModel>

namespace tremotesf
{
    class ModelBatchChanger
    {
    public:
        explicit ModelBatchChanger(QAbstractItemModel* model)
            : model(model) {}

        void changed(int row)
        {
            if (firstRow == -1) {
                reset(row);
            } else {
                if (row == (lastRow + 1)) {
                    lastRow = row;
                } else {
                    changed();
                    reset(row);
                }
            }
        }

        void changed()
        {
            emit model->dataChanged(model->index(firstRow, 0), model->index(lastRow, lastColumn));
        }

    private:
        void reset(int row)
        {
            firstRow = row;
            lastRow = row;
        }

        QAbstractItemModel *const model;
        const int lastColumn = model->columnCount() - 1;

        int firstRow = -1;
        int lastRow = -1;
    };

    class ModelBatchRemover
    {
    public:
        explicit ModelBatchRemover(QAbstractItemModel* model)
            : model(model) {}

        void remove(int row)
        {
            if (lastRow == -1) {
                reset(row);
            } else {
                if (row == (firstRow - 1)) {
                    firstRow = row;
                } else {
                    remove();
                    reset(row);
                }
            }
        }

        void remove()
        {
            model->removeRows(firstRow, lastRow - firstRow + 1);
        }

    private:
        void reset(int row)
        {
            lastRow = row;
            firstRow = row;
        }

        QAbstractItemModel *const model;

        int lastRow = -1;
        int firstRow = -1;
    };
}

#endif // TREMOTESF_MODELUTILS_H
