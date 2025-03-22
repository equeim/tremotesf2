// SPDX-FileCopyrightText: 2015-2025 Alexey Rochev
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef TREMOTESF_EDITLABELSWIDGET_H
#define TREMOTESF_EDITLABELSWIDGET_H

#include <QWidget>

class QComboBox;
class QListWidget;

namespace tremotesf {
    class Rpc;

    class EditLabelsWidget : public QWidget {
        Q_OBJECT
    public:
        explicit EditLabelsWidget(const std::vector<QString>& enabledLabels, Rpc* rpc, QWidget* parent);

        void setFocusOnComboBox();
        bool comboBoxHasFocus() const;

        std::vector<QString> enabledLabels() const;

    private:
        void updateComboBoxLabels();

        Rpc* mRpc;
        QListWidget* mLabelsList{};
        QComboBox* mComboBox{};
    };

} // namespace tremotesf

#endif // TREMOTESF_EDITLABELSWIDGET_H
