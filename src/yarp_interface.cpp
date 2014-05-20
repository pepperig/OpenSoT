/*
 * Copyright: (C) 2014 Walkman Consortium
 * Authors: Enrico Mingo, Alessio Rocchi
 * CopyPolicy: Released under the terms of the GNU GPL v2.0.
*/

#include "yarp_interface.h"
#include "cartesian_utils.h"
#include <yarp/os/Time.h>
#include <iCub/iDynTree/yarp_kdl.h>
#include <drc_shared/yarp_single_chain_interface.h>
using namespace yarp::os;
using namespace yarp::sig;

#define toRad(X) (X*M_PI/180.0)
#define toDeg(X) (X*180.0/M_PI)

void yarp_interface::tic()
{
    time_tic = yarp::os::Time::now();
}

double yarp_interface::toc()
{
    return yarp::os::Time::now() - time_tic;
}

yarp_interface::yarp_interface():left_arm("left_arm","sot_VelKinCon"),right_arm("right_arm","sot_VelKinCon"),torso("torso","sot_VelKinCon"),
                                left_leg("left_leg","sot_VelKinCon"),right_leg("right_leg","sot_VelKinCon")
{
    time_tic = 0.0;

    left_arm_pos_ref_port.open("/sot_VelKinCon/" + walkman::coman::left_arm + "/set_ref:i");
    right_arm_pos_ref_port.open("/sot_VelKinCon/" + walkman::coman::right_arm + "/set_ref:i");
    com_pos_ref_port.open("/sot_VelKinCon/com/set_ref:i");
    clik_port.open("/sot_VelKinCon/set_clik:i");
    world_to_base_link_pose_port.open("/sot_VelKinCon/world_to_base_link_pose:o");
}

yarp_interface::~yarp_interface()
{
    left_arm_pos_ref_port.close();
    right_arm_pos_ref_port.close();
    com_pos_ref_port.close();
    world_to_base_link_pose_port.close();
    clik_port.close();
}

void yarp_interface::getLeftArmCartesianRef(Matrix &left_arm_ref)
{
    yarp::os::Bottle *bot = left_arm_pos_ref_port.read(false);

    if(bot != NULL)
    {
        std::cout<<"Left Arm:"<<std::endl;
        getCartesianRef(left_arm_ref, bot, LOCAL_FRAME_UPPER_BODY);
        std::cout<<std::endl;
    }
}

void yarp_interface::getRightArmCartesianRef(Matrix &right_arm_ref)
{
    yarp::os::Bottle *bot = right_arm_pos_ref_port.read(false);

    if(bot != NULL)
    {
        std::cout<<"Right Arm:"<<std::endl;
        getCartesianRef(right_arm_ref, bot, LOCAL_FRAME_UPPER_BODY);
        std::cout<<std::endl;
    }
}

void yarp_interface::getCoMCartesianRef(Vector &com_ref)
{
    yarp::os::Bottle *bot = com_pos_ref_port.read(false);
    Matrix com_ref_T;
    if(bot != NULL)
    {
        std::cout<<"CoM::"<<std::endl;
        if(getCartesianRef(com_ref_T, bot, LOCAL_FRAME_COM))
        {
            KDL::Frame com_ref_T_KDL;
            YarptoKDL(com_ref_T, com_ref_T_KDL);
            com_ref = KDLtoYarp(com_ref_T_KDL.p);
            std::cout<<std::endl;
        }
    }
}

void yarp_interface::getSetClik(bool &is_clik)
{
    yarp::os::Bottle *bot = clik_port.read(false);

    if(bot != NULL){
        is_clik = (bool)bot->get(0).asInt();
        if(is_clik)
            ROS_WARN("CLIK activated!");
        else
            ROS_WARN("CLIK NOT activated!");
    }
}

bool yarp_interface::sendCartesianRef(BufferedPort<Bottle> &port, const std::string &ref_frame , const Matrix &T)
{
    yarp::os::Bottle &temp = port.prepare();
    yarp::os::Bottle &tf_list  = temp.addList();

    tf_list.addString("frame");
    tf_list.addString(ref_frame);

    yarp::os::Bottle &pose_list  = temp.addList();

    KDL::Frame T_kdl;
    cartesian_utils::fromYARPMatrixtoKDLFrame(T, T_kdl);
    pose_list.addString("data");

    pose_list.add(T_kdl.p.x());
    pose_list.add(T_kdl.p.y());
    pose_list.add(T_kdl.p.z());

    double R = 0.0;
    double P = 0.0;
    double Y = 0.0;
    T_kdl.M.GetRPY(R,P,Y);
    pose_list.add(toDeg(R));
    pose_list.add(toDeg(P));
    pose_list.add(toDeg(Y));

    port.write();
}

bool yarp_interface::getCartesianRef(Matrix &ref, yarp::os::Bottle *bot, const std::string &local_frame)
{
    Bottle& frame = bot->findGroup("frame");
    if(!frame.isNull())
    {
        if(checkRefFrame(frame.get(1).asString(), local_frame.c_str()))
        {
            Bottle& data = bot->findGroup("data");
            if(!data.isNull())
            {
                if(data.size() == 6+1)
                {
                    cartesian_utils::homogeneousMatrixFromRPY(ref,
                                                              data.get(1).asDouble(), data.get(2).asDouble(), data.get(3).asDouble(),
                                                              toRad(data.get(4).asDouble()), toRad(data.get(5).asDouble()), toRad(data.get(6).asDouble()));
                    ROS_INFO("New reference in %s is:", local_frame.c_str());
                    ROS_INFO(ref.toString().c_str());
                    return true;
                }
                else if(data.size() == 7+1)
                {
                    cartesian_utils::homogeneousMatrixFromQuaternion(ref,
                                                                     data.get(1).asDouble(), data.get(2).asDouble(), data.get(3).asDouble(),
                                                                     data.get(4).asDouble(), data.get(5).asDouble(), data.get(6).asDouble(), data.get(7).asDouble());
                    ROS_INFO("New reference in %s is:", local_frame.c_str());
                    ROS_INFO(ref.toString().c_str());
                    return true;
                }
                else
                    ROS_ERROR("wrong size of reference! Should be 6 if RPY are used or 7 if Quaternions are used!");
            }
            else
                ROS_ERROR("no data section in port");
        }
        else
            ROS_ERROR("wrong Reference Frame, should be %s ", local_frame.c_str());
    }
    else
        ROS_ERROR("no frame section in port");
    return false;
}

void yarp_interface::cleanPorts()
{
    yarp::os::Bottle* foo;

    int pendings = left_arm_pos_ref_port.getPendingReads();
    for(unsigned int i = 0; i < pendings; ++i)
        left_arm_pos_ref_port.read(foo);

    pendings = right_arm_pos_ref_port.getPendingReads();
    for(unsigned int i = 0; i < pendings; ++i)
        right_arm_pos_ref_port.read(foo);

    pendings = com_pos_ref_port.getPendingReads();
    for(unsigned int i = 0; i < pendings; ++i)
        com_pos_ref_port.read(foo);

    pendings = clik_port.getPendingReads();
    for(unsigned int i = 0; i < pendings; ++i)
        clik_port.read(foo);

    pendings = world_to_base_link_pose_port.getPendingReads();
    for(unsigned int i = 0; i < pendings; ++i)
        world_to_base_link_pose_port.read(foo);
}
