#include <Python.h>

#include <QPainter>
#include <QDebug>
#include <QStyleOptionGraphicsItem>

#include "ui/depth_image.h"
#include "ui/canvas.h"

DepthImageItem::DepthImageItem(QVector3D pos, QVector3D size, QImage depth,
                               Canvas* canvas)
    : QGraphicsObject(), pos(pos), size(size), depth(depth), canvas(canvas)
{
    connect(canvas, &Canvas::viewChanged, this, &DepthImageItem::reposition);
    reposition();
    setZValue(-20);
}

void DepthImageItem::reposition()
{
    setPos(canvas->worldToScene(pos));
}

QRectF DepthImageItem::boundingRect() const
{
    return QRectF(0, 0, depth.width(), depth.height());
}

void DepthImageItem::paint(QPainter *painter,
                           const QStyleOptionGraphicsItem *option,
                           QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (depth.height() == 0)
    {
        return;
    }

    // We need to transform the depth image into global z space
    QImage depth_ = depth;

    const float czmax = canvas->getZmax();
    const float czmin = canvas->getZmin();

    const QVector3D p = canvas->getMatrix() * pos;

    const float zmax = p.z();
    const float zmin = zmax - size.z();

    const int s = (zmax - zmin) / (czmax - czmin) * 0xff;
    const int o = (zmin - czmin) / (czmax - czmin) * 0xff;
    {
        QPainter p(&depth_);

        // Apply scale
        QImage scale(depth.width(), depth.height(), depth.format());
        scale.fill(s | (s << 8) | (s << 16));
        p.setCompositionMode(QPainter::CompositionMode_Multiply);
        p.drawImage(0, 0, scale);

        // Apply offset
        QImage offset(depth.width(), depth.height(), depth.format());
        offset.fill(o | (o << 8) | (o << 16));
        p.setCompositionMode(QPainter::CompositionMode_Plus);
        p.drawImage(0, 0, offset);

        // Multiply by a basic mask so that we have zeros outside of original
        p.setCompositionMode(QPainter::CompositionMode_Multiply);
        p.drawImage(0, 0,
                    depth.createMaskFromColor(0xff000000, Qt::MaskOutColor));
    }

    painter->setCompositionMode(QPainter::CompositionMode_Lighten);
    painter->drawImage(0, 0, depth_);
}
