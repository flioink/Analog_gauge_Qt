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
#include <QLabel>


int WIDTH = 220;
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

    setAttribute(Qt::WA_TranslucentBackground);
    //setStyleSheet("background:transparent;");

    run_demo_mode();
}

RadialGauge::~RadialGauge()
{}

void RadialGauge::create_close_button()
{
    

    // transparent widget for enforcing click area on transparent buttons. Otherwise Qt event pass through the transparent part.
    //m_click_area = new QWidget(this);
    //m_click_area->setFixedSize(width() * 0.05, height() * 0.06);  // Larger than the X
    //m_click_area->setStyleSheet("background: transparent;");

    // main_widget and gauges_layout have different dimensions - use size constants to position buttons
    m_close_button = new QPushButton("Exit", this);
    m_close_button->setFixedSize(width() * 0.05, height() * 0.06);
    //m_close_button->move(WIDTH * 2 - m_close_button->width() + 20, 10);// top-right
    m_close_button->move(WIDTH * 0.5 - m_close_button->width() * 0.2, HEIGHT * 0.15);

    m_close_button->setStyleSheet(
        "QPushButton {"
        "   background: transparent;"
        "   color: white;"
        "   border: 1px solid rgba(255,255,255,0.3);"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(255,255,255,0.1);"  // hover glow
        "}"
    );    

    //m_click_area->move(m_close_button->pos() - QPoint(0, 0));

    // Forward clicks to the actual button
    //connect(m_click_area, &QWidget::customContextMenuRequested, [this]() { qDebug() << "X AREA CLICKED"; });
    
}



void RadialGauge::create_demo_button()
{
    m_demo_button = new QPushButton("Demo", this);

    // size as percentage of window
    m_demo_button->setFixedSize(width() * 0.1, height() * 0.06); 

    
    //m_demo_button->move(m_demo_button->width() - m_demo_button->width() * 0.7, 10);// top-right   
    m_demo_button->move(WIDTH * 0.5 - m_demo_button->width() * 0.35, HEIGHT * 0.8);  // upper middle of the first gauge 


    m_demo_button->setStyleSheet(
        "QPushButton {"
        "   background: transparent;"
        "   color: white;"
        "   border: 1px solid rgba(255,255,255,0.3);"
        "   border-radius: 3px;"
        "}"
        "QPushButton:hover {"
        "   background: rgba(255,255,255,0.1);"  // hover glow
        "}"
    );
   
}



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
    //m_buttons_area = new QHBoxLayout();      
   
    m_system_monitor = new SystemMonitor(this);
    m_timer = new QTimer(this);

    create_cpu_number_output_label();

    create_cpu_gauge();

    create_memory_gauge();

    //create_disk_gauge();

    create_close_button();
    create_demo_button();
    

    m_timer->start(80); // Update every n milliseconds      

    master_layout->addLayout(m_gauges_area);
    //master_layout->addLayout(m_buttons_area, 1);

    master_layout->setAlignment(Qt::AlignTop);
    this->setCentralWidget(central_widget);    
}

void RadialGauge::create_cpu_number_output_label()
{
    // number output
    m_cpu_load_number = new QLabel("0", this);
    m_cpu_load_number->move(WIDTH * 0.28 + m_cpu_load_number->width()/2, HEIGHT * 0.32);
    QFont font = m_cpu_load_number->font();  // get current font
    font.setPointSize(24);           // set new size (in points)
    m_cpu_load_number->setFont(font);        // apply it back
}

void RadialGauge::create_cpu_gauge()
{
    // CPU gauge
    m_cpu_gauge = new AnalogGauge(-132.5, 2.65, "./gauge_cpu.png", this);
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


            if (!m_paused)
            {
                m_cpu_gauge->set_speed(cpu);

                QString value_as_string = QString::number(cpu, 'f', 0);
                
                m_cpu_load_number->setText(value_as_string);
            }

        });

}


void RadialGauge::create_memory_gauge()
{

    // memory gauge
    m_memory_gauge = new AnalogGauge(-153, 3.05, "./gauge_ram.png", this);
    m_memory_gauge->setMinimumSize(WIDTH, WIDTH);
    // add gauge object to layout
    m_gauges_area->addWidget(m_memory_gauge);

    connect(m_timer, &QTimer::timeout, this, [this]()
        {
            auto mem_perc_used = m_system_monitor->get_memory_usage();

            qDebug() << "MEMORY % USED: " << m_system_monitor->get_memory_usage();

            if (!m_paused)
            {
                m_memory_gauge->set_speed(mem_perc_used);
            }
        });

}

// not in use
void RadialGauge::create_disk_gauge()
{
    m_disk_gauge = new AnalogGauge(-135, 2.6, "./gauge0c.png", this);
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

           // qDebug() << "DISK SPEED in MB: " << clamped;

            if (!m_paused)
            {
                m_disk_gauge->set_speed(mb_per_sec);
            }
        });

}


void RadialGauge::connect_button_signals()
{ 
  

   connect(m_demo_button, &QPushButton::clicked, this, [this](int value) { run_demo_mode();} );  

   connect(m_close_button, &QPushButton::clicked, this, &QMainWindow::close);

}


void RadialGauge::run_demo_mode()
{
    m_demo_button->setEnabled(false);
    
    m_cpu_gauge->move_needle();
    m_memory_gauge->move_needle();
    //m_disk_gauge->move_needle();
    m_paused = true;

    QTimer::singleShot(2200, this, [this]()
        {
            m_demo_button->setEnabled(true);
            m_paused = false;
        }
    );
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
    painter.drawImage((-m_needle_width / 2), -m_needle_height + fudge, m_needle);

   /* painter.setBrush(Qt::red);
    painter.drawEllipse(QPoint(0, 0 - fudge), arrow_width, arrow_length);*/

    painter.setBrush(Qt::black);
    painter.drawEllipse(QPoint(0, 0), cover_cap_radius, cover_cap_radius);

    
}

void AnalogGauge::set_needle_pivot()
{
    
    m_needle.load("./gauge_arrow.png");
    m_needle = m_needle.scaledToHeight(WIDTH * 0.5, Qt::SmoothTransformation);

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
    int animation_duration = 1000;    

    int pause_duration = 100;
    
    sweep->setDuration(animation_duration); // animation length in miliseconds
    sweep->setStartValue(m_current_angle); // starting angle 

    //if (m_end_position > 0) // prevents weird behavior if the arrow is set to rotate counter clock-wise
    //{
    //    sweep->setKeyValueAt(0.7, 100);  // overshoot
    //}    

    sweep->setKeyValueAt(1, m_end_position);     // settle
    sweep->setEasingCurve(QEasingCurve::OutCubic);  // Smooth deceleration into settle

    // Return to start  
    QPropertyAnimation* retreat = new QPropertyAnimation(this, "current_angle");
    retreat->setEasingCurve(QEasingCurve::InOutQuad);
    retreat->setDuration(animation_duration);
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
    if (m_current_angle != angle) // check if the value actually changed
    {         
        m_current_angle = angle;
        update(); // tells Qt to repaint the widget

        emit current_angle_changed(m_current_angle); 
    }

}

void AnalogGauge::load_bg(const QString& bg)
{
    m_background.load(bg);
}



// MONITOR CLASS
// 
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
    PdhAddCounter(m_disk_query, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", 0, &m_disk_counter);// does not show anything useful

    
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


