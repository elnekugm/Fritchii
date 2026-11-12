// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QWidget>

namespace FritchiiUi {

/// The background of the home screen: the sky of Frutiger Aero behind the glass. It paints a
/// gradient with the sun in the corner, a few bubbles and a streak of light over it.
class AeroBackground : public QWidget {
public:
    explicit AeroBackground(QWidget* parent = nullptr);

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent* event) override;
};

} // namespace FritchiiUi
