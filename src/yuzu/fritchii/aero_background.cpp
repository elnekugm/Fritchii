// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "yuzu/fritchii/aero_background.h"

#include <cmath>

#include <QPainter>
#include <QPaintEvent>
#include <QLinearGradient>
#include <QRadialGradient>

namespace FritchiiUi {

namespace {

// The bubbles of the sky, as a share of the size of the widget.
struct Bubble {
    qreal x;
    qreal y;
    qreal r;
    qreal alpha;
};

// The same bubbles on every size, so the sky does not change with the window.
constexpr Bubble Bubbles[] = {
    {0.12, 0.22, 0.075, 0.16}, {0.78, 0.16, 0.045, 0.13}, {0.88, 0.62, 0.090, 0.11},
    {0.22, 0.78, 0.055, 0.12}, {0.62, 0.88, 0.038, 0.10}, {0.46, 0.12, 0.028, 0.09},
    {0.34, 0.52, 0.034, 0.08}, {0.70, 0.40, 0.022, 0.07},
};

} // namespace

AeroBackground::AeroBackground(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_StyledBackground, false);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    // The sky is drawn with soft edges, so the widget itself is translucent.
    setAttribute(Qt::WA_TranslucentBackground, true);
}

QSize AeroBackground::sizeHint() const {
    return {640, 420};
}

void AeroBackground::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRectF area{0.0, 0.0, static_cast<qreal>(width()), static_cast<qreal>(height())};

    // The sky: a light blue at the top that deepens towards the bottom.
    QLinearGradient sky{area.topLeft(), area.bottomLeft()};
    sky.setColorAt(0.0, QColor{0xBF, 0xE4, 0xFA});
    sky.setColorAt(0.45, QColor{0x8F, 0xCD, 0xF2});
    sky.setColorAt(1.0, QColor{0x4C, 0xA3, 0xE0});
    painter.fillRect(area, sky);

    // The sun in the upper left corner.
    QRadialGradient sun{area.left() + area.width() * 0.18, area.top() + area.height() * 0.14,
                        area.width() * 0.55};
    sun.setColorAt(0.0, QColor{255, 255, 255, 120});
    sun.setColorAt(0.45, QColor{255, 252, 220, 55});
    sun.setColorAt(1.0, QColor{255, 255, 255, 0});
    painter.fillRect(area, sun);

    // The bubbles of the sky.
    for (const auto& bubble : Bubbles) {
        const QPointF centre{area.left() + area.width() * bubble.x,
                             area.top() + area.height() * bubble.y};
        const qreal radius{std::min(area.width(), area.height()) * bubble.r};
        QRadialGradient glass{centre.x() - radius * 0.3, centre.y() - radius * 0.35, radius};
        glass.setColorAt(0.0, QColor{255, 255, 255, static_cast<int>(bubble.alpha * 255 * 1.6)});
        glass.setColorAt(0.55, QColor{255, 255, 255, static_cast<int>(bubble.alpha * 255 * 0.5)});
        glass.setColorAt(1.0, QColor{255, 255, 255, 0});
        painter.setBrush(glass);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(centre, radius, radius);
    }

    // The streak of light over the sky.
    painter.setPen(Qt::NoPen);
    QLinearGradient light{area.topLeft(), area.bottomRight()};
    light.setColorAt(0.0, QColor{255, 255, 255, 40});
    light.setColorAt(0.35, QColor{255, 255, 255, 0});
    painter.setBrush(light);
    QPolygonF streak;
    streak << area.topLeft() << area.topLeft() + QPointF{area.width() * 0.55, 0.0}
           << area.topLeft() + QPointF{area.width() * 0.22, area.height()}
           << area.topLeft() + QPointF{0.0, area.height()};
    painter.drawPolygon(streak);
}

} // namespace FritchiiUi
