#!/usr/bin/env python

"""
@package folding_assemly_contoller
@file contact_estimated_broadcast.py
@author Anthony Remazeilles
@brief tool for displaying the output of the contact estimator in rviz.
Copyright (C) 2016 Tecnalia Research and Innovation
Distributed under the GNU GPL v3.
For full terms see https://www.gnu.org/licenses/gpl.txt
"""

import rospy
from folding_assembly_controller.msg import *
import tf

broadcast = tf.TransformBroadcaster()

def callback(msg):
    print "received message: {}".format(msg)

    rot = tuple((0, 0, 0, 1))
    pt = msg.feedback.contact_point_estimate
    trans = tuple((pt.x, pt.y, pt.z))
    broadcast.sendTransform(trans, rot, msg.header.stamp, 'estimate', 's2')
    pt = msg.feedback.contact_point_computed
    trans = tuple((pt.x, pt.y, pt.z))
    broadcast.sendTransform(trans, rot, msg.header.stamp, 'computed', 's2')


if __name__ == '__main__':
    rospy.init_node('contact_display', anonymous=True)
    topic_name = "/estimator_node/feedback"
    rospy.Subscriber(topic_name, EstimateActionFeedback, callback)

    rospy.spin()
