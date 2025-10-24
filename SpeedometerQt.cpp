#include "SpeedometerQt.h"
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDebug>
#include <QPropertyAnimation>

#include<windows.h>

int WIDTH = 400;
int HEIGHT = WIDTH;

double needle_start = 180;
double test_rotation_range = 2.5; // max angle  

RadialGauge::RadialGauge(QWidget* parent)
    : QMainWindow(parent)
{   

    //setFixedSize(WIDTH, HEIGHT);
    build_UI(); 

    connect_signals();

}

RadialGauge::~RadialGauge()
{}

void RadialGauge::build_UI()
{

    // create the master layout
    QWidget* central_widget = new QWidget(this);
    QVBoxLayout* master_layout = new QVBoxLayout(central_widget);

    QVBoxLayout* speedometer_area = new QVBoxLayout();
    QVBoxLayout* buttons_area = new QVBoxLayout();

    m_main_slider = new QSlider(Qt::Horizontal, this);
    
    m_main_slider->setRange(0, 100);
   
    m_main_button = new QPushButton("Button 1", this);

    // custom gauge
    m_speed_gauge = new AnalogGauge(needle_start, test_rotation_range, this);
    m_speed_gauge->setMinimumSize(WIDTH, WIDTH);

    
    
    // add gauge object 
    speedometer_area->addWidget(m_speed_gauge);
    
    
    buttons_area->addWidget(m_main_button);
    buttons_area->addWidget(m_main_slider);

    

    master_layout->addLayout(speedometer_area, 5);
    master_layout->addLayout(buttons_area, 1);


    master_layout->setAlignment(Qt::AlignTop);
    this->setCentralWidget(central_widget);

}

void RadialGauge::connect_signals()
{ 
   // connect the slider from the "stage" class to the "gauge" class method that sets speed 
   connect(m_main_slider, &QSlider::valueChanged, this, [this](int value){ m_speed_gauge->set_speed(value); });

   connect(m_main_button, &QPushButton::clicked, this, [this](int value) { m_speed_gauge->move_needle(); });

}



int fudge = 53;
// Gauge class

AnalogGauge::AnalogGauge(double needle_start_pos, double max_range, QWidget* parent):
    m_current_angle(needle_start_pos), m_rotation_range(max_range), m_remap_value(needle_start_pos)
{   
    
    load_assets();
}

void AnalogGauge::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
   
    // draw background 
    painter.drawImage(rect(), m_background);     

    // transforms for needle
   
    painter.translate(m_center_x, m_center_y);  // move origin to gauge center
    painter.rotate(m_current_angle);         // rotate coordinate system

    // needle is drawn in transformed coordinates
    //painter.drawImage((-m_needle_width / 2), -m_needle_height + fudge, m_needle);

    painter.setBrush(Qt::red);
    painter.drawEllipse(QPoint(0, 0 - fudge), 5, 70);

    
}

void AnalogGauge::load_assets()
{
    m_background.load("./speed_gauge.jpg");
    m_needle.load("./speed_arrow.png");
    //m_needle = m_needle.scaledToHeight(100, Qt::SmoothTransformation);

    // needle pivot
    m_center_x = WIDTH / 2;
    m_center_y = HEIGHT / 2;

    m_needle_width = m_needle.width();
    m_needle_height = m_needle.height();

    
}

void AnalogGauge::set_speed(double speed)
{
    this->m_current_angle = map_speed_to_angle(speed); 

    update(); // call repaint 

    qDebug() << "Current angle" << m_current_angle;
}

double AnalogGauge::map_speed_to_angle(int speed) 
{
    qDebug() << "rot range: " << m_rotation_range;
    return speed * m_rotation_range + m_remap_value; // maps the range
       
}



void AnalogGauge::move_needle()
{

    QPropertyAnimation* animation = new QPropertyAnimation(this, "current_angle");
    
    animation->setDuration(5000); // animation length in miliseconds
    animation->setStartValue(-180); // starting angle
    animation->setKeyValueAt(0.7, 100);  // Overshoot
    animation->setKeyValueAt(1, 80);     // Settle
    animation->setEasingCurve(QEasingCurve::InOutQuad); // smooth motion
    animation->start(QAbstractAnimation::DeleteWhenStopped); // clean up when done


}

void AnalogGauge::set_current_angle(double angle)
{
    if (m_current_angle != angle) 
      { // Check if the value actually changed
        m_current_angle = angle;
        update(); // Critical: This tells Qt to repaint the widget
        emit current_angle_changed(m_current_angle); // Emit the signal if you declared one
    }

}
