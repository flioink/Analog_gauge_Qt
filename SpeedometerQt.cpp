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
#include <QMouseEvent>
#include <QPoint>


int WIDTH = 160;
int HEIGHT = WIDTH;



RadialGauge::RadialGauge(QWidget* parent)
    : QMainWindow(parent)
{           
    build_UI();
   
    setWindowFlags(Qt::FramelessWindowHint);

    adjustSize();
    setFixedSize(size());
    connect_button_signals();  

    setWindowFlags(windowFlags() | Qt::WindowStaysOnTopHint);
}

RadialGauge::~RadialGauge()
{}

//mouse events overrides
void RadialGauge::mousePressEvent(QMouseEvent* event) 
{
    if (event->button() == Qt::LeftButton) // if left button pressed
    {
        // get current mouse position and subtract the topleft corner position from it
        m_drag_position = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
    
}

void RadialGauge::mouseMoveEvent(QMouseEvent* event) 
{
    if (event->buttons() & Qt::LeftButton)
    {
        // on drag - offset position from the mouse position to avoid jumps 
        move(event->globalPosition().toPoint() - m_drag_position);
        event->accept();
    }
}

void RadialGauge::build_UI()
{

    // create the master layout
    QWidget* central_widget = new QWidget(this);
    QVBoxLayout* master_layout = new QVBoxLayout(central_widget);

    m_gauges_area = new QHBoxLayout();
    m_buttons_area = new QHBoxLayout();

    /*m_main_slider1 = new QSlider(Qt::Horizontal, this);    
    m_main_slider1->setRange(0, 100);*/    
   
    m_main_button = new QPushButton("Gauge Test", this);
    m_system_monitor = new SystemMonitor(this);
    m_timer = new QTimer(this);

    create_cpu_gauge();

    create_memory_gauge();

    create_disk_gauge();  
    

    m_timer->start(100); // Update every n milliseconds       
        
    
    m_buttons_area->addWidget(m_main_button);

    master_layout->addLayout(m_gauges_area, 5);
    master_layout->addLayout(m_buttons_area, 1);


    master_layout->setAlignment(Qt::AlignTop);
    this->setCentralWidget(central_widget);

}


void RadialGauge::create_cpu_gauge()
{
    // CPU gauge
    m_cpu_gauge = new AnalogGauge(-180, 2.6, "./gauge0.png", this);
    m_cpu_gauge->setMinimumSize(WIDTH, WIDTH);
    // add gauge object to layout
    m_gauges_area->addWidget(m_cpu_gauge);


    connect(m_timer, &QTimer::timeout, this, [this]()
        {
            double cpu = m_system_monitor->get_cpu_usage();
            qDebug() << "CURRENT CPU LOAD: " << cpu;

            if (cpu < 1)
            {
                cpu = m_last_good_cpu_value; // keep previous value if current falls to zero to prevent jitter
            }
            else m_last_good_cpu_value = cpu;


            if (!m_pause)
            {
                m_cpu_gauge->set_speed(cpu);
            }

        });

}


void RadialGauge::create_memory_gauge()
{

    // memory gauge
    m_memory_gauge = new AnalogGauge(-155, 3.0, "./gauge3.jpg", this);
    m_memory_gauge->setMinimumSize(WIDTH, WIDTH);
    // add gauge object to layout
    m_gauges_area->addWidget(m_memory_gauge);

    connect(m_timer, &QTimer::timeout, this, [this]()
        {
            auto mem_perc_used = m_system_monitor->get_memory_usage();

            qDebug() << "MEMORY % USED: " << m_system_monitor->get_memory_usage();

            if (!m_pause)
            {
                m_memory_gauge->set_speed(mem_perc_used);
            }
        });

}


void RadialGauge::create_disk_gauge()
{
    m_disk_gauge = new AnalogGauge(-135, 2.6, "./gauge4.jpg", this);
    m_disk_gauge->setMinimumSize(WIDTH, WIDTH);
    // add gauge object to layout
    m_gauges_area->addWidget(m_disk_gauge);

    double end_point = m_disk_gauge->get_gauge_end_position(); // this is the gauge end point based on m_end_position = 100 * m_rotation_range + m_remap_value

    connect(m_timer, &QTimer::timeout, this, [this, end_point]()
        {

            auto disk_speed = m_system_monitor->get_disk_read_speed();

            double mb_per_sec = disk_speed / (1024 * 1024);

            if (mb_per_sec < 0.1) mb_per_sec = 0.0;

            // limit the needle so it doesn't go crazy at very high values
            double clamped = std::min(mb_per_sec, end_point);

            qDebug() << "DISK SPEED in MB: " << clamped;

            if (!m_pause)
            {
                m_disk_gauge->set_speed(mb_per_sec);
            }
        });

}


void RadialGauge::connect_button_signals()
{ 
   // connect the slider from the "stage" class to the "gauge" class method that sets speed 
   connect(m_main_slider1, &QSlider::valueChanged, this, [this](int value){ m_cpu_gauge->set_speed(value); });
   

   connect(m_main_button, &QPushButton::clicked, this, [this](int value)
       {
           m_main_button->setEnabled(false);
           m_pause = true;
           m_cpu_gauge->move_needle(); 
           m_memory_gauge->move_needle();
           m_disk_gauge->move_needle();

           QTimer::singleShot( 2000, this, [this]() 
               { 
                m_main_button->setEnabled(true);
                m_pause = false; 
               } 
           );
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
    m_end_position = 100 * m_rotation_range + m_remap_value;

    load_bg(bg);
    
    set_needle_pivot();
}

void AnalogGauge::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
   
    // draw background 
    painter.drawImage(rect(), m_background);     

    // transforms for needle
   
    painter.translate(m_gauge_center_x, m_gauge_center_y);  // move origin to gauge center
    painter.rotate(m_current_angle);         // rotate coordinate system

    // needle is drawn in transformed coordinates
    //painter.drawImage((-m_needle_width / 2), -m_needle_height + fudge, m_needle);

    painter.setBrush(Qt::red);
    painter.drawEllipse(QPoint(0, 0 - fudge), arrow_width, arrow_length);

    painter.setBrush(Qt::black);
    painter.drawEllipse(QPoint(0, 0), cover_cap_radius, cover_cap_radius);

    
}

void AnalogGauge::set_needle_pivot()
{
    
    m_needle.load("./speed_arrow.png");
    //m_needle = m_needle.scaledToHeight(100, Qt::SmoothTransformation);

    // needle pivot is set at the center of the circle
    m_gauge_center_x = WIDTH / 2;
    m_gauge_center_y = HEIGHT / 2;

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

    

    int pause_duration = 100;
    
    sweep->setDuration(duration); // animation length in miliseconds
    sweep->setStartValue(m_current_angle); // starting angle sets the direction

    if (m_end_position > 0) // prevents weird behavior if the arrow is set to rotate counter clock-wise
    {
        sweep->setKeyValueAt(0.7, 100);  // overshoot
    }
    

    sweep->setKeyValueAt(1, m_end_position);     // settle
    sweep->setEasingCurve(QEasingCurve::OutCubic);  // Smooth deceleration into settle

    // Return to start  
    QPropertyAnimation* retreat = new QPropertyAnimation(this, "current_angle");
    retreat->setEasingCurve(QEasingCurve::InOutQuad);
    retreat->setDuration(duration);
    retreat->setStartValue(m_end_position);
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
// Declare a query - PDH_HQUERY my_query;
// Open the query - PdhCollectQueryData(my_query);
// Declare a counter for my_query - PDH_HCOUNTER my_counter;
// Set up the counter - PdhAddCounter(my_query, L"\\Processor(_Total)\\% Processor Time", 0, &my_counter);
// Get the recorded value in a usable format - PdhGetFormattedCounterValue(my_query, PDH_FMT_DOUBLE, NULL, &value);

SystemMonitor::SystemMonitor(QObject* parent): QObject(parent), m_smooth_cpu(0.0), m_smooth_disk(0.0)
{
    // Pdh queries
    PdhOpenQuery(NULL, 0, &m_cpu_query);
    PdhAddCounter(m_cpu_query, L"\\Processor(_Total)\\% Processor Time", 0, &m_cpu_counter);

    PdhOpenQuery(NULL, 0, &m_memory_query);
    PdhAddCounter(m_memory_query, L"\\Memory\\Available MBytes", 0, &m_memory_counter);

    PdhOpenQuery(NULL, 0, &m_disk_query);
    PdhAddCounter(m_disk_query, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &m_disk_counter);

    // these don't show anything
    //PdhAddCounter(m_disk_query, L"\\LogicalDisk(_Total)\\Disk Read Bytes / sec", 0, &m_disk_counter);
    //PdhAddCounter(m_disk_query, L"\\LogicalDisk(C:)\\Disk Read Bytes / sec", 0, &m_disk_counter);
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
    m_smooth_cpu = 0.9 * m_smooth_cpu + current_cpu * 0.1; // low pass
        
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

double SystemMonitor::get_disk_read_speed()
{
    PdhCollectQueryData(m_disk_query);
    PDH_FMT_COUNTERVALUE value;
    PdhGetFormattedCounterValue(m_disk_counter, PDH_FMT_DOUBLE, NULL, &value);

    double current_disk_usage = value.doubleValue;    

    m_smooth_disk = 0.9 * m_smooth_disk + current_disk_usage * 0.1;

    //qDebug() << "Cirrent Disk in Kb/s: " << current_disk_usage;

    return m_smooth_disk;

}


