#include "elevator.h"
#include <QDebug>

void Elevator::setElevatorX(int _x)
{
    m_eleX = _x;
    Q_EMIT elevatorXChanged();
}

void Elevator::setElevatorY(int _y)
{
    m_eleY = _y;
    Q_EMIT elevatorYChanged();
}

void Elevator::setValidUse(bool _valid)
{
    m_validUse = _valid;
    Q_EMIT validUseChanged();
}

void Elevator::setWidth(int _width)
{
    m_width = _width;
    Q_EMIT widthChanged();
}

Elevator::Elevator(QObject* parent)
    :QObject(parent)
{
    doorState = CLOSE;
    doorButton = NONE;
    elevatorState = STOP;
    liftDirection = SUSPEND;

    elevatorHeight.nowHeight = 0;
    doorWidth.nowWidth = 0;

    memset(floorDownList, 0, sizeof(floorDownList));
    memset(floorUpList, 0, sizeof(floorUpList));
    memset(panelButtonList, 0, sizeof(panelButtonList));

    thread1 = std::thread(&Elevator::elevatorThread, this);
    thread3 = std::thread(&Elevator::doorThread, this);
}

Elevator::~Elevator()
{
    thread1.join();
    thread3.join();
    thread2.join();
    thread4.join();
}

void Elevator::elevatorThread()
{
    std::cout<<"this is elevator thread"<<std::endl;
    while (true)
    {
        switch (elevatorState)
        {
        case UP:
            liftUp();
            break;
        case DOWN:
            liftDown();
            break;
        case STOP:
            liftStop();
            break;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return;
}

void Elevator::doorThread()
{
    while (true)
    {
        switch (doorState)
        {
        case CLOSE:
            doorClose();
            break;
        case LOCK:
            doorLock();
            break;
        case OPENING:
            doorOpening();
            break;
        case CLOSING:
            doorClosing();
            break;
        case OPEN:
            doorOpen();
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return;
}

void Elevator::doorClose()
{
    while (doorState == CLOSE)
    {
        if (doorButton == OPENDOOR)
        {
            doorState = OPENING;
            return;
        }

        if ((elevatorState == UP) || (elevatorState == DOWN))
        {
            doorState = LOCK;
            return;
        }

        if (doorButton == CLOSEDOOR)
        {
            doorButton = NONE;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}


void Elevator::doorLock()
{
    while (true)
    {
        if (elevatorState == STOP)
        {
            doorState = OPENING;
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Elevator::doorOpening()
{
    while (true)
    {
        if (doorWidth.nowWidth < doorWidth.fullWidth)
        {
            doorWidth.nowWidth += doorWidth.openSpeed;
        }
        if (doorWidth.nowWidth == doorWidth.fullWidth)
        {
            if (doorButton == OPENDOOR)
                doorButton = NONE;
            doorState = OPEN;
            return;
        }

        if (doorButton == CLOSEDOOR)
        {
            doorState = CLOSING;
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Elevator::doorClosing()
{
    while (true)
    {
        if (doorState == CLOSING)
        {
            doorWidth.nowWidth -= doorWidth.openSpeed;
        }

        if (doorWidth.nowWidth == 0)
        {
            doorState = CLOSE;
            return;
        }

        if (doorButton == OPENDOOR)
        {
            doorState = OPENING;
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void Elevator::doorOpen()
{
    int	 delay = doorWidth.waitTime;
    while (delay > 0)
    {
        if (doorButton == CLOSEDOOR)
        {
            doorState = CLOSING;
            return;
        }
        if (doorButton == OPENDOOR)
        {
            delay = doorWidth.waitTime;
            continue;
        }
        delay--;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    doorState = CLOSING;
    if (doorButton == OPENDOOR)
    {
        doorButton = NONE;
    }
}

void Elevator::liftDown()
{
    int level;
    do {
        elevatorHeight.nowHeight -= elevatorHeight.moveSpeed;
        level = elevatorHeight.nowHeight / elevatorHeight.levelHigh;

        int i;
        for (i = level; i >0; i--)
        {
            if ((floorDownList[level] == 1) || (panelButtonList[level] == 1))
            {
                break;
            }
        }

        if (elevatorHeight.nowHeight % elevatorHeight.levelHigh == 0)
        {
            if ((floorDownList[level] == 1) || (panelButtonList[level] == 1))
            {
                floorDownList[level] = 0;
                panelButtonList[level] = 0;

                elevatorState = STOP;
                return;
            }
            if (i == elevatorHeight.fullLevel)
            {
                if (floorUpList[level] == 1)
                {
                    floorUpList[level] = 0;

                    elevatorState = STOP;
                    return;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (true);
}

void Elevator::liftUp()
{
    int level;
    do {
        elevatorHeight.nowHeight += elevatorHeight.moveSpeed;
        if (elevatorHeight.nowHeight >= 5500)
            elevatorHeight.nowHeight = 5500;
        level = elevatorHeight.nowHeight / elevatorHeight.levelHigh;

        int i;
        for ( i = level + 1; i < elevatorHeight.fullLevel; i++)
        {
            if ((floorUpList[i] == 1) || (panelButtonList[i] == 1))
            {
                break;
            }
        }

        if (elevatorHeight.nowHeight % elevatorHeight.levelHigh == 0)
        {
            if ((floorUpList[level] == 1) || (panelButtonList[level] == 1))
            {
                floorUpList[level] = 0;
                panelButtonList[level] = 0;

                elevatorState = STOP;
                return;
            }
            if (i == elevatorHeight.fullLevel)
            {
                if (floorDownList[level] == 1)
                {
                    floorDownList[level] = 0;

                    elevatorState = STOP;
                    return;
                }
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (true);
}

void Elevator::liftStop()
{
    while (true)
    {
        int level = elevatorHeight.nowHeight / elevatorHeight.levelHigh;
        if (doorState != CLOSE)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        if (liftDirection == LIFT)
        {
            for (int i = level + 1; i < elevatorHeight.fullLevel; i++)
            {
                if (floorUpList[i] || panelButtonList[i])
                {
                    liftDirection = LIFT;
                    elevatorState = UP;
                    return;
                }
            }
            for (int i = elevatorHeight.fullLevel; i > level ; i--)
            {
                if (floorDownList[i])
                {
                    liftDirection = LIFT;
                    elevatorState = UP;
                    return;
                }
            }
            floorDownList[level] = 0;
            liftDirection = SUSPEND;
        }
        else if (liftDirection == DROP)
        {
            for (int i = 0; i < level; i++)
            {
                if (floorDownList[i] || panelButtonList[i])
                {
                    liftDirection = DROP;
                    elevatorState = DOWN;
                    return;
                }
            }
            for (int i = 0; i < level; i++)
            {
                if (floorUpList[i])
                {
                    liftDirection = DROP;
                    elevatorState = DOWN;
                    return;
                }
            }
            liftDirection = SUSPEND;
        }
        else
        {
            if (floorDownList[level] || floorUpList[level] || panelButtonList[level])
            {
                floorDownList[level] = 0;
                floorUpList[level] = 0;
                panelButtonList[level] = 0;
                continue;
            }
            for (int i = level + 1; i < elevatorHeight.fullLevel; i++)
            {
                if (floorDownList[i] || floorUpList[i] || panelButtonList[i])
                {
                    liftDirection = LIFT;
                }
            }
            for (int i = level - 1; i >= 0; i--)
            {
                if (floorDownList[i] || floorUpList[i] || panelButtonList[i])
                {
                    liftDirection = DROP;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void Elevator::startElevator()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    while (true)
    {
        this->on_elevatorBtn_clicked();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return;
}

void Elevator::syncDoorWidth()
{
    int width1;
    while (true)
    {
        width1 = doorWidth.nowWidth/10;
        if (width1 != 0)
        {
            this->on_leftDoor_clicked();
            this->on_rightDoor_clicked();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return;
}

void Elevator::on_startBtn_clicked()
{
    std::cout<<"start elevator here"<<std::endl;
    thread2 = std::thread(&Elevator::startElevator, this);
    thread4 = std::thread(&Elevator::syncDoorWidth, this);
    this->setValidUse(true);
}

void Elevator::on_closeBtn_clicked()
{
    this->setValidUse(false);
}

void Elevator::addPanelDownList(int floor)
{
    floorDownList[floor - 1] = 1;
}

void Elevator::addPanelList(int floor)
{
    panelButtonList[floor - 1] = 1;
}

void Elevator::addPanelUpList(int floor)
{
    floorUpList[floor - 1] = 1;
}

void Elevator::on_elevatorBtn_clicked()
{
    std::cout<<"count height"<<std::endl;
    height = elevatorHeight.nowHeight/10;
    level = elevatorHeight.nowHeight/elevatorHeight.levelHigh + 1;
    std::cout<<"height = "<<height<< " and level = "<<level<<std::endl;
    std::cout<<"state : "<<elevatorState<<std::endl;
    m_eleX = 300;
    m_eleY = 550 - height;
    std::cout << "height in execute function = ["<< m_eleX << " , " << m_eleY << "]" << std::endl;
    this->setElevatorX(m_eleX);
    this->setElevatorY(m_eleY);
}

void Elevator::on_leftDoor_clicked()
{
    width1 = doorWidth.nowWidth/10;
    this->setWidth(width1);
}

void Elevator::on_rightDoor_clicked()
{
    width1 = doorWidth.nowWidth/10;
    this->setWidth(width1);
}
