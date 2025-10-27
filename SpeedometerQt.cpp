#include "SpeedometerQt.h"
#include <QSlider>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QPainter>
#include <QDebug>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>
#include <QTimer>

#include<windows.h>

int WIDTH = 220;
int HEIGHT = WIDTH;

double test_needle_start = -180;
double test_rotation_range = 2.6; // max angle  

RadialGauge::RadialGauge(QWidget* parent)
    : QMainWindow(parent)
{           
    build_UI();
    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);

    adjustSize();
    setFixedSize(size());
    connect_signals();   
}

RadialGauge::~RadialGauge()
{}

void RadialGauge::build_UI()
{

    // create the master layout
    QWidget* central_widget = new QWidget(this);
    QVBoxLayout* master_layout = new QVBoxLayout(central_widget);

    QHBoxLayout* gauges_area = new QHBoxLayout();
    QHBoxLayout* buttons_area = new QHBoxLayout();

    m_main_slider1 = new QSlider(Qt::Horizontal, this);
   
    
    m_main_slider1->setRange(0, 100);
    
   
    m_main_button = new QPushButton("Gauge Test", this);

    m_system_monitor = new SystemMonitor(this);
    QTimer* timer = new QTimer(this);


    // custom gauge
    m_cpu_gauge = new AnalogGauge(test_needle_start, test_rotation_range, "./gauge0.png", this);
    m_cpu_gauge->setMinimumSize(WIDTH, WIDTH); 
    // add gauge object to layout
    gauges_area->addWidget(m_cpu_gauge);


    connect(timer, &QTimer::timeout, this, [this]() 
     { 
        double cpu = m_system_monitor->get_cpu_usage();
        qDebug() << "CURRENT CPU LOAD: " << cpu;
        
        if (cpu < 1)
        { 
            cpu = m_last_good_value; // keep previous value if current falls to zero to prevent jitter
        } 
        else m_last_good_value = cpu;

        

        m_cpu_gauge->set_speed(cpu);  

        qDebug() << "MEMORY % USED: " << m_system_monitor->get_memory_usage();
            
     });


    // custom gauge
    m_memory_gauge = new AnalogGauge(-155, 3.0, "./gauge3.jpg", this);
    m_memory_gauge->setMinimumSize(WIDTH, WIDTH);
    // add gauge object to layout
    gauges_area->addWidget(m_memory_gauge);

    connect(timer, &QTimer::timeout, this, [this]()
        {

            auto mem_perc_used = m_system_monitor->get_memory_usage();

            qDebug() << "MEMORY % USED: " << m_system_monitor->get_memory_usage();



            m_memory_gauge->set_speed(mem_perc_used);

            

        });

    

    timer->start(60); // Update every n milliseconds   
    
    
    buttons_area->addWidget(m_main_slider1);    
    
    buttons_area->addWidget(m_main_button);

    master_layout->addLayout(gauges_area, 5);
    master_layout->addLayout(buttons_area, 1);


    master_layout->setAlignment(Qt::AlignTop);
    this->setCentralWidget(central_widget);

}

void RadialGauge::connect_signals()
{ 
   // connect the slider from the "stage" class to the "gauge" class method that sets speed 
   connect(m_main_slider1, &QSlider::valueChanged, this, [this](int value){ m_cpu_gauge->set_speed(value); });
   /*connect(m_main_slider2, &QSlider::valueChanged, this, [this](int value){ m_speed_gauge_2->set_speed(value); });
   connect(m_main_slider3, &QSlider::valueChanged, this, [this](int value){ m_speed_gauge_3->set_speed(value); });*/

   connect(m_main_button, &QPushButton::clicked, this, [this](int value)
       {
           m_main_button->setEnabled(false);
           m_cpu_gauge->move_needle(); 

           QTimer::singleShot( 2000, this, [this]() { m_main_button->setEnabled(true);} );
       });


   

}



int fudge = WIDTH * 0.15;
int arrow_length = WIDTH * 0.16;
int arrow_width = WIDTH * 0.015;
int cover_cap_radius = WIDTH * 0.04;


// Gauge class

AnalogGauge::AnalogGauge(double needle_start_pos, double max_range, QString bg, QWidget* parent):
    m_current_angle(needle_start_pos), m_rotation_range(max_range), m_remap_value(needle_start_pos)
{   
    load_bg(bg);
    
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
    painter.drawEllipse(QPoint(0, 0 - fudge), arrow_width, arrow_length);

    painter.setBrush(Qt::black);
    painter.drawEllipse(QPoint(0, 0), cover_cap_radius, cover_cap_radius);

    
}

void AnalogGauge::load_assets()
{
    
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

    //qDebug() << "Current angle" << m_current_angle;
}

double AnalogGauge::map_speed_to_angle(int speed) 
{   
    return speed * m_rotation_range + m_remap_value; // maps the range       
}



void AnalogGauge::move_needle()
{
    

    //m_main_button
    QSequentialAnimationGroup* group = new QSequentialAnimationGroup(this);

    QPropertyAnimation* sweep = new QPropertyAnimation(this, "current_angle");
    int duration = 1000;

    double end_position = 100 * m_rotation_range + m_remap_value;

    int pause_duration = 100;
    
    sweep->setDuration(duration); // animation length in miliseconds
    sweep->setStartValue(m_current_angle); // starting angle sets the direction

    if (end_position > 0) // prevents weird behavior if the arrow is set to rotate counter clock-wise
    {
        sweep->setKeyValueAt(0.7, 100);  // overshoot
    }
    

    sweep->setKeyValueAt(1, end_position);     // settle
    sweep->setEasingCurve(QEasingCurve::OutCubic);  // Smooth deceleration into settle

    // Return to start  
    QPropertyAnimation* retreat = new QPropertyAnimation(this, "current_angle");
    retreat->setEasingCurve(QEasingCurve::InOutQuad);
    retreat->setDuration(duration);
    retreat->setStartValue(end_position);
    retreat->setEndValue(m_current_angle);

    group->addAnimation(sweep);
    group->addPause(100);
    group->addAnimation(retreat);
    group->start(QAbstractAnimation::DeleteWhenStopped); // clean up when done

    
}

// this is for the animation macro system from the header
void AnalogGauge::set_current_angle(double angle)
{
    if (m_current_angle != angle) 
    { 
        // check if the value actually changed
        m_current_angle = angle;
        update(); // tells Qt to repaint the widget

        emit current_angle_changed(m_current_angle); 
    }

}

void AnalogGauge::load_bg(const QString& bg)
{
    m_background.load(bg);
}


// system monitor class uses the Pdh header

SystemMonitor::SystemMonitor(QObject* parent): QObject(parent)
{
    PdhOpenQuery(NULL, 0, &m_cpu_query);
    PdhAddCounter(m_cpu_query, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpu_counter);

    PdhOpenQuery(NULL, 0, &m_memory_query);
    PdhAddCounter(m_memory_query, L"\\Memory\\Available MBytes", 0, &m_memory_counter);
}

SystemMonitor::~SystemMonitor()
{
}

double SystemMonitor::get_cpu_usage()
{
    PdhCollectQueryData(m_cpu_query);
    PDH_FMT_COUNTERVALUE value;
    PdhGetFormattedCounterValue(m_cpu_counter, PDH_FMT_DOUBLE, NULL, &value);

    double current_cpu = value.doubleValue;

    //qDebug() << "CURRENT CPU LOAD: " << current_cpu;

    // low pass
    m_smooth_cpu = 0.9 * m_smooth_cpu + current_cpu * 0.1;
        
    return m_smooth_cpu;
}

double SystemMonitor::get_memory_usage()
{

    PdhCollectQueryData(m_memory_query);
    PDH_FMT_COUNTERVALUE value;
    PdhGetFormattedCounterValue(m_memory_counter, PDH_FMT_DOUBLE, NULL, &value);

    // get total memory to calculate percentage
    MEMORYSTATUSEX mem_info;
    mem_info.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&mem_info);

    DWORDLONG total_memory = mem_info.ullTotalPhys / (1024 * 1024); // convert to MB
    double available_MB = value.doubleValue;
    double used_memory_percentage = ((total_memory - available_MB) / total_memory) * 100.0;

    

    return used_memory_percentage;
}


// 1. "Start recording performance data"
//PdhCollectQueryData(query); 

// 2. "Get the recorded value in a usable format"  
//PdhGetFormattedCounterValue(counter, PDH_FMT_DOUBLE, NULL, &value);
// "Give me that snapshot as a double"