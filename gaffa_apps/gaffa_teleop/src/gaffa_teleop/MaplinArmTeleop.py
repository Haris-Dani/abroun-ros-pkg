#! /usr/bin/python

# ROS imports
import roslib
roslib.load_manifest( 'gaffa_teleop' )
import rospy

import sys
import math
import os.path
import time

import yaml
import pygtk
pygtk.require('2.0')
import gtk
import gobject

import sensor_msgs.msg
from maplin_arm.ROS_ArmClient import ROS_ArmClient
from maplin_arm.ArmDriver import MotorStates

#-------------------------------------------------------------------------------
class MainWindow:

    BASE_MOTOR_IDX = 0
    WRIST_MOTOR_IDX = 3

    #---------------------------------------------------------------------------
    def __init__( self ):
    
        self.scriptPath = os.path.dirname( __file__ )
        self.cameraImagePixBuf = None
            
        # Connect to the robot via ROS
        rospy.init_node( 'MaplinArmTeleop', anonymous=True )
        
        self.cameraImageTopic = rospy.Subscriber( "/camera/image", 
            sensor_msgs.msg.Image, self.cameraImageCallback )
        self.rosArmClient = ROS_ArmClient()
        
        self.upButtonPressed = False
        self.downButtonPressed = False
        self.leftButtonPressed = False
        self.rightButtonPressed = False
        
        self.guideImagePixBuf = gtk.gdk.pixbuf_new_from_file( 
            self.scriptPath + "/../../config/OpticalFlowPos.png" )
            
        # Setup the GUI        
        builder = gtk.Builder()
        builder.add_from_file( self.scriptPath + "/GUI/MaplinArmTeleop.glade" )
        
        self.window = builder.get_object( "winMain" )   
        self.dwgCameraImage = builder.get_object( "dwgCameraImage" )
        
        builder.connect_signals( self )
               
        updateLoop = self.update()
        gobject.idle_add( updateLoop.next )
        
        self.window.show()
        
    #---------------------------------------------------------------------------
    def onWinMainDestroy( self, widget, data = None ):  
        gtk.main_quit()
        
    #---------------------------------------------------------------------------   
    def main( self ):
        # All PyGTK applications must have a gtk.main(). Control ends here
        # and waits for an event to occur (like a key press or mouse event).
        gtk.main()
            
    #---------------------------------------------------------------------------
    def cameraImageCallback( self, rosImage ):
        
        if rosImage.encoding == "rgb8" or rosImage.encoding == "bgr8":
            
            # Display the image
            self.cameraImagePixBuf = gtk.gdk.pixbuf_new_from_data( 
                rosImage.data, 
                gtk.gdk.COLORSPACE_RGB,
                False,
                8,
                rosImage.width,
                rosImage.height,
                rosImage.step )
            self.cameraImagePixBuf.add_alpha( False, 0, 0, 0 )
                
            # Resize the drawing area if necessary
            if self.dwgCameraImage.get_size_request() != ( rosImage.width, rosImage.height ):
                self.dwgCameraImage.set_size_request( rosImage.width, rosImage.height )

            # Add the guide
            self.guideImagePixBuf.composite( self.cameraImagePixBuf, 0, 0, 
                self.guideImagePixBuf.get_width(), self.guideImagePixBuf.get_height(), 
                0, 0, 1.0, 1.0, gtk.gdk.INTERP_NEAREST, 255 )

            self.dwgCameraImage.queue_draw()
            self.lastImage = rosImage

        else:
            rospy.logerr( "Unhandled image encoding - " + rosImage.encoding )
    
    #---------------------------------------------------------------------------
    def onBtnUpPressed( self, widget ):
        self.upButtonPressed = True
        
    #---------------------------------------------------------------------------
    def onBtnDownPressed( self, widget ):
        self.downButtonPressed = True
    
    #---------------------------------------------------------------------------
    def onBtnLeftPressed( self, widget ):
        self.leftButtonPressed = True
        
    #---------------------------------------------------------------------------
    def onBtnRightPressed( self, widget ):
        self.rightButtonPressed = True
        
    #---------------------------------------------------------------------------
    def onBtnUpReleased( self, widget ):
        self.upButtonPressed = False
        
    #---------------------------------------------------------------------------
    def onBtnDownReleased( self, widget ):
        self.downButtonPressed = False
    
    #---------------------------------------------------------------------------
    def onBtnLeftReleased( self, widget ):
        self.leftButtonPressed = False
        
    #---------------------------------------------------------------------------
    def onBtnRightReleased( self, widget ):
        self.rightButtonPressed = False
    
    #---------------------------------------------------------------------------
    def onDwgCameraImageExposeEvent( self, widget, data = None ):
    
        if self.cameraImagePixBuf != None:
            
            imgRect = self.getImageRectangleInWidget( widget,
                self.cameraImagePixBuf.get_width(), self.cameraImagePixBuf.get_height() )
                
            imgOffsetX = imgRect.x
            imgOffsetY = imgRect.y
                
            # Get the total area that needs to be redrawn
            imgRect = imgRect.intersect( data.area )
        
            srcX = imgRect.x - imgOffsetX
            srcY = imgRect.y - imgOffsetY
        
            widget.window.draw_pixbuf( widget.get_style().fg_gc[ gtk.STATE_NORMAL ],
                self.cameraImagePixBuf, srcX, srcY, 
                imgRect.x, imgRect.y, imgRect.width, imgRect.height )

    #---------------------------------------------------------------------------
    def getImageRectangleInWidget( self, widget, imageWidth, imageHeight ):
        
        # Centre the image inside the widget
        widgetX, widgetY, widgetWidth, widgetHeight = widget.get_allocation()
        
        imgRect = gtk.gdk.Rectangle( 0, 0, widgetWidth, widgetHeight )
        
        if widgetWidth > imageWidth:
            imgRect.x = (widgetWidth - imageWidth) / 2
            imgRect.width = imageWidth
            
        if widgetHeight > imageHeight:
            imgRect.y = (widgetHeight - imageHeight) / 2
            imgRect.height = imageHeight
        
        return imgRect
        
    #---------------------------------------------------------------------------
    def update( self ):

        UPDATE_RATE = 30.0  # Updates per second

        lastTime = time.time()

        while 1:
            
            curTime = time.time()
            if curTime - lastTime > 1.0/UPDATE_RATE:
                
                # Work out whether the motors should be off or going forward/backwards
                baseMotorState = MotorStates.OFF
                wristMotorState = MotorStates.OFF
                
                if self.leftButtonPressed and not self.rightButtonPressed:
                    baseMotorState = MotorStates.BACKWARD
                elif self.rightButtonPressed and not self.leftButtonPressed:
                    baseMotorState = MotorStates.FORWARD
                    
                if self.upButtonPressed and not self.downButtonPressed:
                    wristMotorState = MotorStates.FORWARD
                elif self.downButtonPressed and not self.upButtonPressed:
                    wristMotorState = MotorStates.BACKWARD
                
                # Send the motor states to the arm
                self.rosArmClient.setArmMotorStates(
                    [ ( self.BASE_MOTOR_IDX, baseMotorState ),
                    ( self.WRIST_MOTOR_IDX, wristMotorState ) ] )                      

                # Save the update time
                lastTime = curTime
                
            yield True
            
        yield False
        
        
#-------------------------------------------------------------------------------
if __name__ == "__main__":

    mainWindow = MainWindow()
    mainWindow.main()