#include "serigraph_widget.h"
#include <QPainter>
#include <QPaintEvent>

serigraph_widget::serigraph_widget(QWidget *parent) 
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Dark);
    setAutoFillBackground(true);
}

void serigraph_widget::set_source_image(const QImage &image) {
    m_image = image;
    
    // Resize widget to match image dimensions
    this->setFixedSize(m_image.size());
    
    update();
}

void serigraph_widget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    if (m_image.isNull()) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Go to File > Open to load an image");
    } else {
        // FUTURE: Your Mixbox / Solver rendering logic goes here
        painter.drawImage(0, 0, m_image);
    }
}