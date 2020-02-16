#include "rawglyphwidget.h"
#include <QPainter>
#include <QDebug>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QStyleOptionGraphicsItem>
#include <QElapsedTimer>

#include <iostream>

static constexpr qreal gridSize = 20;

RawGlyphWidget::RawGlyphWidget(const Font::Glyph& glyph, QGraphicsItem* parent) :
    QGraphicsWidget(parent),
    glyph_ { glyph }
{
    setFocusPolicy(Qt::ClickFocus);
    setPreferredSize({ gridSize * static_cast<qreal>(glyph.size().width),
                       gridSize * static_cast<qreal>(glyph.size().height) });
}

void RawGlyphWidget::load(const Font::Glyph &glyph)
{
    glyph_ = glyph;
    setPreferredSize({ gridSize * static_cast<qreal>(glyph.size().width),
                       gridSize * static_cast<qreal>(glyph.size().height) });
    update();
}

QRectF RawGlyphWidget::boundingRect() const
{
    return QRectF(-0.25,
                  -0.25,
                  glyph_.size().width * gridSize + 0.25,
                  glyph_.size().height * gridSize + 0.25);
}

void RawGlyphWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    QElapsedTimer timer;
    timer.start();

    QRectF rect = QRectF(0, 0,
                         glyph_.size().width * gridSize,
                         glyph_.size().height * gridSize);

    painter->setPen(QPen(QBrush(Qt::darkGray), 0.5));

    painter->fillRect(rect, Qt::white);
    painter->drawRect(rect);

    for (std::size_t row = 0; row < glyph_.size().height; ++row) {
        for (std::size_t col = 0; col < glyph_.size().width; ++col) {
            if (glyph_.is_pixel_set({col, row})) {
                painter->fillRect(QRectF(QPointF(col * gridSize, row * gridSize),
                                         QSizeF(gridSize, gridSize)), Qt::black);
            }
        }
    }

    for (qreal w = 0; w < rect.width(); w += gridSize) {
        painter->drawLine(QLineF(QPointF(w, rect.top()), QPointF(w, rect.bottom())));
    }
    for (qreal h = 0; h < rect.height(); h += gridSize) {
        painter->drawLine(QLineF(QPointF(rect.left(), h), QPointF(rect.right(), h)));
    }
    if (focusedPixel_.has_value()) {
        painter->setPen(QPen(QBrush(Qt::red), 1));
        painter->drawRect(QRectF(QPointF(focusedPixel_->x * gridSize, focusedPixel_->y * gridSize),
                                 QSizeF(gridSize, gridSize)));
    }

    qDebug() << __FUNCTION__ << option->exposedRect << boundingRect()
             << option->exposedRect.toRect().contains(boundingRect().toRect())
             << QString("(%1ms)").arg(timer.elapsed());
}

void RawGlyphWidget::keyPressEvent(QKeyEvent *event)
{

}

void RawGlyphWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (!isDuringMouseMove_) {
        return;
    }

    if (event->key() == Qt::Key_Alt || event->key() == Qt::Key_AltGr) {
        penState_ = true;
    }
}

void RawGlyphWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    handleMousePress(event);
}

void RawGlyphWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    handleMousePress(event);
}

void RawGlyphWidget::handleMousePress(QGraphicsSceneMouseEvent *event)
{
    Font::Point currentPixel = pointForEvent(event);
    qDebug() << event << currentPixel.x << currentPixel.y;

    penState_ = !event->modifiers().testFlag(Qt::AltModifier);
    affectedItems_.clear();
    affectedItems_[currentPixel] = penState_;
    isDuringMouseMove_ = true;

    glyph_.set_pixel_set(currentPixel, penState_);
    emit pixelChanged(currentPixel, penState_);

    auto previousPixel = focusedPixel_;
    focusedPixel_ = currentPixel;
    updateIfNeeded(UpdateMode::UpdateFocusAndPixels, previousPixel);
}

void RawGlyphWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    Font::Point currentPixel = pointForEvent(event);

    auto previousPixel = focusedPixel_;
    focusedPixel_ = currentPixel;
    auto updateMode = UpdateMode::UpdateFocus;

    // If item not visited or visited with a different state
    if (affectedItems_.find(currentPixel) == affectedItems_.end() ||
            affectedItems_[currentPixel] != penState_)
    {
        qDebug() << "mouse move to new item" << currentPixel.x << currentPixel.y << penState_;
        updateMode = UpdateMode::UpdateFocusAndPixels;
        affectedItems_[currentPixel] = penState_;

        glyph_.set_pixel_set(currentPixel, penState_);
        emit pixelChanged(currentPixel, penState_);
    }
    updateIfNeeded(updateMode, previousPixel);
}

void RawGlyphWidget::updateIfNeeded(UpdateMode updateMode, std::optional<Font::Point> previousFocusedPixel)
{
    QRectF rect;

    if (focusedPixel_.has_value()
            && previousFocusedPixel.has_value()
            && *focusedPixel_ == *previousFocusedPixel
            && updateMode == UpdateMode::UpdateFocus)
    {
        // no change in focus so no redraw
        return;
    }

    if (focusedPixel_.has_value()) {
        rect = QRectF(QPointF(focusedPixel_->x * gridSize - 0.5, focusedPixel_->y * gridSize -0.5),
                      QSizeF(gridSize + 1, gridSize + 1));
    }
    if (previousFocusedPixel.has_value()) {
        auto previousRect = QRectF(QPointF(previousFocusedPixel->x * gridSize - 0.5, previousFocusedPixel->y * gridSize - 0.5),
                                   QSizeF(gridSize + 1, gridSize + 1));
        if (rect.isValid()) {
            rect = rect.united(previousRect);
        } else {
            rect = previousRect;
        }
    }

    if (rect.isValid()) {
        update(rect);
    }
}

void RawGlyphWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    isDuringMouseMove_ = false;

    for (const auto& i : affectedItems_) {
        std::cout << "(" << i.first.x << ", " << i.first.y << ") ";
    }
    std::cout << std::endl;
}

Font::Point RawGlyphWidget::pointForEvent(QGraphicsSceneMouseEvent *event) const
{
    auto row = static_cast<std::size_t>(std::max(event->pos().y() / gridSize, 0.0));
    auto col = static_cast<std::size_t>(std::max(event->pos().x() / gridSize, 0.0));
    row = std::min(row, glyph_.size().height-1);
    col = std::min(col, glyph_.size().width-1);

    return { col, row };
}
