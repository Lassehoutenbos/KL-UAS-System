#include "mapwidget.h"
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsPathItem>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPixmap>
#include <QBrush>
#include <QPen>
#include <QMenu>
#include <QAction>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QApplication>
#include <cmath>

MapWidget::MapWidget(QWidget *parent)
    : QGraphicsView(parent)
    , centerLat(52.3676)  // Amsterdam coordinates
    , centerLon(4.9041)
    , mapScale(1.0)
    , selectedWaypoint(-1)
{
    setupUI();
    setupContextMenu();
    createMapBackground();
}

void MapWidget::setupUI()
{
    // Create graphics scene
    mapScene = new QGraphicsScene(this);
    mapScene->setSceneRect(-500, -500, 1000, 1000);
    setScene(mapScene);
    
    // Configure view
    setRenderHint(QPainter::Antialiasing);
    setDragMode(QGraphicsView::RubberBandDrag);
    setMouseTracking(true);
    
    // Initialize graphics items
    droneIcon = nullptr;
    droneHeading = nullptr;
    homeIcon = nullptr;
    flightPath = nullptr;
}

void MapWidget::setupContextMenu()
{
    contextMenu = new QMenu(this);
    
    addWaypointAction = new QAction("Add Waypoint", this);
    removeWaypointAction = new QAction("Remove Waypoint", this);
    clearWaypointsAction = new QAction("Clear All Waypoints", this);
    zoomInAction = new QAction("Zoom In", this);
    zoomOutAction = new QAction("Zoom Out", this);
    resetZoomAction = new QAction("Reset Zoom", this);
    
    contextMenu->addAction(addWaypointAction);
    contextMenu->addAction(removeWaypointAction);
    contextMenu->addAction(clearWaypointsAction);
    contextMenu->addSeparator();
    contextMenu->addAction(zoomInAction);
    contextMenu->addAction(zoomOutAction);
    contextMenu->addAction(resetZoomAction);
    
    connect(removeWaypointAction, &QAction::triggered, this, &MapWidget::removeSelectedWaypoint);
    connect(clearWaypointsAction, &QAction::triggered, this, &MapWidget::clearAllWaypoints);
    connect(zoomInAction, &QAction::triggered, this, &MapWidget::zoomIn);
    connect(zoomOutAction, &QAction::triggered, this, &MapWidget::zoomOut);
    connect(resetZoomAction, &QAction::triggered, this, &MapWidget::resetZoom);
}

void MapWidget::createMapBackground()
{
    // Create a simple grid-based map background
    QPixmap mapPixmap(1000, 1000);
    mapPixmap.fill(QColor(200, 220, 200)); // Light green background
    
    QPainter painter(&mapPixmap);
    painter.setPen(QPen(QColor(150, 150, 150), 1));
    
    // Draw grid
    for (int i = 0; i <= 1000; i += 50) {
        painter.drawLine(i, 0, i, 1000);
        painter.drawLine(0, i, 1000, i);
    }
    
    // Draw major grid lines
    painter.setPen(QPen(QColor(100, 100, 100), 2));
    for (int i = 0; i <= 1000; i += 100) {
        painter.drawLine(i, 0, i, 1000);
        painter.drawLine(0, i, 1000, i);
    }
    
    // Add some terrain features
    painter.setBrush(QBrush(QColor(100, 150, 255))); // Water blue
    painter.setPen(QPen(QColor(80, 120, 200), 2));
    
    // River
    QPainterPath river;
    river.moveTo(100, 200);
    river.quadTo(300, 150, 500, 300);
    river.quadTo(700, 450, 900, 400);
    painter.drawPath(river);
    
    // Lake
    painter.drawEllipse(600, 600, 200, 150);
    
    // Forest areas
    painter.setBrush(QBrush(QColor(50, 150, 50))); // Dark green
    painter.setPen(QPen(QColor(30, 100, 30), 1));
    painter.drawEllipse(200, 700, 150, 100);
    painter.drawEllipse(700, 200, 120, 80);
    
    // Urban area
    painter.setBrush(QBrush(QColor(180, 180, 180))); // Gray
    painter.setPen(QPen(QColor(120, 120, 120), 1));
    painter.drawRect(400, 400, 200, 150);
    
    // Add compass rose
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(QBrush(Qt::white));
    QPointF center(900, 100);
    
    // North arrow
    QPolygonF northArrow;
    northArrow << center + QPointF(0, -30) << center + QPointF(-10, -10) 
               << center << center + QPointF(10, -10);
    painter.drawPolygon(northArrow);
    
    painter.drawText(center + QPointF(-5, -35), "N");
    painter.drawText(center + QPointF(15, 5), "E");
    painter.drawText(center + QPointF(-5, 25), "S");
    painter.drawText(center + QPointF(-25, 5), "W");
    
    mapBackground = mapScene->addPixmap(mapPixmap);
    mapBackground->setPos(-500, -500);
}

void MapWidget::updateDronePosition(double lat, double lon, double heading)
{
    QPointF dronePos = latLonToScene(lat, lon);
    
    // Create or update drone icon
    if (!droneIcon) {
        droneIcon = mapScene->addEllipse(-8, -8, 16, 16, QPen(Qt::red, 2), QBrush(Qt::yellow));
        droneHeading = mapScene->addLine(0, 0, 0, -20, QPen(Qt::red, 3));
    }
    
    droneIcon->setPos(dronePos);
    droneHeading->setPos(dronePos);
    
    // Update heading indicator
    QTransform transform;
    transform.rotate(heading);
    droneHeading->setTransform(transform);
    
    // Update flight path
    updateFlightPath(lat, lon);
    
    // Center view on drone
    centerOn(dronePos);
}

void MapWidget::addWaypoint(double lat, double lon)
{
    QPointF pos = latLonToScene(lat, lon);
    
    // Create waypoint visual
    QGraphicsEllipseItem *waypoint = mapScene->addEllipse(-6, -6, 12, 12, 
        QPen(Qt::blue, 2), QBrush(Qt::cyan));
    waypoint->setPos(pos);
    waypoint->setFlag(QGraphicsItem::ItemIsSelectable, true);
    
    // Add waypoint number
    QGraphicsTextItem *numberText = mapScene->addText(QString::number(waypoints.size() + 1));
    numberText->setPos(pos + QPointF(8, -8));
    numberText->setDefaultTextColor(Qt::blue);
    
    waypoints.append(waypoint);
    waypointPositions.append(QPointF(lat, lon));
    
    updateWaypointConnections();
    emit waypointAdded(lat, lon);
}

void MapWidget::clearWaypoints()
{
    for (auto waypoint : waypoints) {
        mapScene->removeItem(waypoint);
        delete waypoint;
    }
    
    for (auto connection : waypointConnections) {
        mapScene->removeItem(connection);
        delete connection;
    }
    
    waypoints.clear();
    waypointConnections.clear();
    waypointPositions.clear();
}

void MapWidget::setHomePosition(double lat, double lon)
{
    QPointF homePos = latLonToScene(lat, lon);
    
    // Create or update home icon
    if (!homeIcon) {
        homeIcon = mapScene->addEllipse(-10, -10, 20, 20, QPen(Qt::green, 3), QBrush(Qt::lightGray));
        
        // Add home symbol (house shape)
        QGraphicsLineItem *roof1 = mapScene->addLine(-8, -2, 0, -10, QPen(Qt::green, 2));
        QGraphicsLineItem *roof2 = mapScene->addLine(0, -10, 8, -2, QPen(Qt::green, 2));
        QGraphicsRectItem *house = mapScene->addRect(-6, -2, 12, 8, QPen(Qt::green, 2));
        
        roof1->setParentItem(homeIcon);
        roof2->setParentItem(homeIcon);
        house->setParentItem(homeIcon);
    }
    
    homeIcon->setPos(homePos);
}

void MapWidget::updateFlightPath(double lat, double lon)
{
    flightPathPoints.append(latLonToScene(lat, lon));
    
    // Limit flight path to last 100 points
    if (flightPathPoints.size() > 100) {
        flightPathPoints.removeFirst();
    }
    
    updateFlightPathDisplay();
}

void MapWidget::updateFlightPathDisplay()
{
    if (flightPath) {
        mapScene->removeItem(flightPath);
        delete flightPath;
    }
    
    if (flightPathPoints.size() < 2) return;
    
    QPainterPath path;
    path.moveTo(flightPathPoints.first());
    
    for (int i = 1; i < flightPathPoints.size(); ++i) {
        path.lineTo(flightPathPoints[i]);
    }
    
    flightPath = mapScene->addPath(path, QPen(Qt::magenta, 2, Qt::DashLine));
}

void MapWidget::updateWaypointConnections()
{
    // Clear existing connections
    for (auto connection : waypointConnections) {
        mapScene->removeItem(connection);
        delete connection;
    }
    waypointConnections.clear();
    
    // Create new connections
    for (int i = 0; i < waypoints.size() - 1; ++i) {
        QPointF start = latLonToScene(waypointPositions[i].x(), waypointPositions[i].y());
        QPointF end = latLonToScene(waypointPositions[i + 1].x(), waypointPositions[i + 1].y());
        
        QGraphicsLineItem *connection = mapScene->addLine(start.x(), start.y(), end.x(), end.y(),
            QPen(Qt::blue, 2, Qt::DotLine));
        waypointConnections.append(connection);
    }
}

QPointF MapWidget::latLonToScene(double lat, double lon)
{
    // Simple conversion for demo purposes
    // In a real application, use proper map projection
    double x = (lon - centerLon) * 10000 * mapScale;
    double y = -(lat - centerLat) * 10000 * mapScale; // Negative because screen Y increases downward
    
    return QPointF(x, y);
}

QPair<double, double> MapWidget::sceneToLatLon(QPointF point)
{
    double lat = centerLat - (point.y() / (10000 * mapScale));
    double lon = centerLon + (point.x() / (10000 * mapScale));
    
    return QPair<double, double>(lat, lon);
}

void MapWidget::wheelEvent(QWheelEvent *event)
{
    // Zoom functionality
    const double scaleFactor = 1.15;
    
    if (event->angleDelta().y() > 0) {
        scale(scaleFactor, scaleFactor);
        mapScale *= scaleFactor;
    } else {
        scale(1.0 / scaleFactor, 1.0 / scaleFactor);
        mapScale /= scaleFactor;
    }
}

void MapWidget::contextMenuEvent(QContextMenuEvent *event)
{
    QPointF scenePos = mapToScene(event->pos());
    QPair<double, double> latLon = sceneToLatLon(scenePos);
    
    // Store the position for potential waypoint addition
    disconnect(addWaypointAction, &QAction::triggered, nullptr, nullptr);
    connect(addWaypointAction, &QAction::triggered, [this, latLon]() {
        addWaypoint(latLon.first, latLon.second);
    });
    
    contextMenu->exec(event->globalPos());
}

void MapWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Handle waypoint selection
        QGraphicsItem *item = itemAt(event->pos());
        selectedWaypoint = -1;
        
        for (int i = 0; i < waypoints.size(); ++i) {
            if (waypoints[i] == item) {
                selectedWaypoint = i;
                break;
            }
        }
    }
    
    QGraphicsView::mousePressEvent(event);
}

void MapWidget::removeSelectedWaypoint()
{
    if (selectedWaypoint >= 0 && selectedWaypoint < waypoints.size()) {
        mapScene->removeItem(waypoints[selectedWaypoint]);
        delete waypoints[selectedWaypoint];
        waypoints.removeAt(selectedWaypoint);
        waypointPositions.removeAt(selectedWaypoint);
        
        updateWaypointConnections();
        emit waypointRemoved(selectedWaypoint);
        selectedWaypoint = -1;
    }
}

void MapWidget::clearAllWaypoints()
{
    clearWaypoints();
}

void MapWidget::zoomIn()
{
    scale(1.25, 1.25);
    mapScale *= 1.25;
}

void MapWidget::zoomOut()
{
    scale(0.8, 0.8);
    mapScale *= 0.8;
}

void MapWidget::resetZoom()
{
    resetTransform();
    mapScale = 1.0;
}
