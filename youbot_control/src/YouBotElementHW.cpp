//
// Created by sham on 11.04.23.
//

#include "YouBotElementHW.h"

namespace yb {


InterfaceSwitcher gen_switcher(const std::vector<std::string>& controllers_name){
    InterfaceSwitcher res;
    for(const auto& name : controllers_name){
        res[name] = false;
    }
    return res;
}


void print_interface_switcher(const InterfaceSwitcher &i_switcher) {
    for (const auto &i: i_switcher) {
        ROS_INFO("%s is %i", i.first.c_str(), i.second);
    }
}


Joint::Joint() :
    position(0),
    velocity(0),
    effort(0),
    torque(0),
    absolute_position(0) {}


std::vector<yb::Joint> gen_joints(std::vector<std::string> joint_names) {
    std::vector<yb::Joint> joints(joint_names.size());
    for (int i = 0; i < joints.size(); ++i) {
        joints.at(i).name = joint_names.at(i);
    }
    return joints;
}


void print_joints(const std::vector<yb::Joint> &joints) {
    for (const auto &i: joints) {
        std::cout << i.name << " "
                  << i.position << " "
                  << i.velocity << " "
                  << i.effort << " "
                  << i.torque << " "
                  << i.absolute_position << " "
                  << std::endl;
    }
}


void ElementHW::doSwitch(const std::list<hardware_interface::ControllerInfo> &start_list,
                             const std::list<hardware_interface::ControllerInfo> &stop_list) {
    for (auto &controller_it: stop_list) {
        if (cmd_switcher_.count(controller_it.name))
            cmd_switcher_[controller_it.name] = false;
    }

    for (auto &controller_it: start_list) {
        if (cmd_switcher_.count(controller_it.name))
            cmd_switcher_.at(controller_it.name) = true;
    }
}


bool YouBotBaseHW::init(const ros::NodeHandle &root_nh, const ros::NodeHandle &robot_hw_nh) {
    std::string config_name;
    std::string config_path;
    root_nh.param("config_name", config_name, config_name);
    root_nh.param("config_path", config_path, config_path);
    youBotBase = std::make_unique<youbot::YouBotBase>(config_name, config_path);
    youBotBase->doJointCommutation();

    std::vector<std::string> joints_names(BASEJOINTS);
    root_nh.param("base_hardware_interface/joints", joints_names, joints_names);
    joints_sensed_ = gen_joints(joints_names);
    joints_cmd_ = joints_sensed_;

    std::vector<std::string> controllers_name = {
            "youbot_base/joint_state_controller",
            "youbot_base/joints_eff_controller",
            "youbot_base/joints_vel_controller",
            "youbot_base/joints_pos_controller",
            "youbot_base/mecanum_drive_controller"
    };
    cmd_switcher_ = gen_switcher(controllers_name);

    return true;
}


void YouBotBaseHW::read(const ros::Time &time, const ros::Duration &period) {
    static std::vector<youbot::JointSensedAngle> jointSensedAngle(BASEJOINTS);
    static std::vector<youbot::JointSensedVelocity> jointSensedVelocity(BASEJOINTS);
    static std::vector<youbot::JointSensedCurrent> jointSensedCurrent(BASEJOINTS);
    static std::vector<youbot::JointSensedTorque> jointSensedTorque(BASEJOINTS);

    youBotBase->getJointData(jointSensedAngle);
    youBotBase->getJointData(jointSensedVelocity);
    youBotBase->getJointData(jointSensedCurrent);
    youBotBase->getJointData(jointSensedTorque);

    for (int i = 0; i < BASEJOINTS; ++i) {
        joints_sensed_.at(i).position = jointSensedAngle.at(i).angle.value();
        joints_sensed_.at(i).velocity = jointSensedVelocity.at(i).angularVelocity.value();
        joints_sensed_.at(i).effort = jointSensedCurrent.at(i).current.value();
        joints_sensed_.at(i).torque = jointSensedTorque.at(i).torque.value();
    }
}


void YouBotBaseHW::write(const ros::Time &time, const ros::Duration &period) {
    static std::vector<youbot::JointCurrentSetpoint> jointsCurren(BASEJOINTS);
    static std::vector<youbot::JointVelocitySetpoint> jointsVelocity(BASEJOINTS);
    static std::vector<youbot::JointAngleSetpoint> jointsAngle(BASEJOINTS);

    if (cmd_switcher_.at("youbot_base/mecanum_drive_controller")) {
        for (int i = 0; i < BASEJOINTS; ++i) {
            jointsVelocity.at(i).angularVelocity = joints_cmd_.at(i).velocity * radian_per_second;
        }
        youBotBase->setJointData(jointsVelocity);
    } else if (cmd_switcher_.at("youbot_base/joints_eff_controller")) {
        for (int i = 0; i < BASEJOINTS; ++i) {
            jointsCurren.at(i).current = joints_cmd_.at(i).effort * ampere;
        }
        youBotBase->setJointData(jointsCurren);
    } else if (cmd_switcher_.at("youbot_base/joints_vel_controller")) {
        for (int i = 0; i < BASEJOINTS; ++i) {
            jointsVelocity.at(i).angularVelocity = joints_cmd_.at(i).velocity * radian_per_second;
        }
        youBotBase->setJointData(jointsVelocity);
    } else if (cmd_switcher_.at("youbot_base/joints_pos_controller")) {
        for (int i = 0; i < BASEJOINTS; ++i) {
            jointsAngle.at(i).angle = joints_cmd_.at(i).position * radians;
        }
        youBotBase->setJointData(jointsAngle);
    }
}

bool YouBotArmHW::init(const ros::NodeHandle &root_nh, const ros::NodeHandle &robot_hw_nh) {
    std::string config_name;
    std::string config_path;
    root_nh.param("config_name", config_name, config_name);
    root_nh.param("config_path", config_path, config_path);
    youBotManipulator = std::make_unique<youbot::YouBotManipulator>(config_name, config_path);
    youBotManipulator->doJointCommutation();
    youBotManipulator->calibrateManipulator();
    youBotManipulator->calibrateGripper();

    std::vector<std::string> joints_names;
    std::vector<std::string> gripper_names;
    root_nh.param("arm_hardware_interface/joints", joints_names, joints_names);
    root_nh.param("arm_hardware_interface/gripper", gripper_names, gripper_names);
    joints_names.insert(joints_names.end(), gripper_names.begin(), gripper_names.end());
    joints_sensed_ = gen_joints(joints_names);
    joints_cmd_ = joints_sensed_;

    std::vector<double> start_pos{0.101, 0.0101, -0.016, 0.023, 0.12};
    for(int i = 0; i < ARMJOINTS; i++){
        joints_cmd_.at(i).position = start_pos.at(i);
    }
    joints_cmd_.at(GRIPPERBAR1).position = 0.01;
    joints_cmd_.at(GRIPPERBAR2).position = 0.01;

    print_joints(joints_sensed_);
    std::vector<std::string> controllers_name = {
            "youbot_arm/joint_state_controller",
            "youbot_arm/joints_eff_controller",
            "youbot_arm/joints_vel_controller",
            "youbot_arm/joints_pos_controller",
            "youbot_arm/gripper_pos_controller"
    };
    cmd_switcher_ = gen_switcher(controllers_name);

    return true;
}

void YouBotArmHW::read(const ros::Time &time, const ros::Duration &period) {
    static std::vector<youbot::JointSensedAngle> jointSensedAngle(ARMJOINTS);
    static std::vector<youbot::JointSensedVelocity> jointSensedVelocity(ARMJOINTS);
    static std::vector<youbot::JointSensedCurrent> jointSensedCurrent(ARMJOINTS);
    static std::vector<youbot::JointSensedTorque> jointSensedTorque(ARMJOINTS);
    // static std::vector<youbot::GripperSensedBarPosition> GripperSensedBarPosition(GRIPPERJOINTS);
    // static std::vector<youbot::GripperSensedVelocity> gripper(GRIPPERJOINTS);
    // static std::vector<youbot::GripperSensedBarSpacing> GripperSensedBarPosition(GRIPPERJOINTS);
    // static std::vector<youbot::GripperSensedBarPosition> GripperSensedBarPosition(GRIPPERJOINTS);

    youBotManipulator->getJointData(jointSensedAngle);
    youBotManipulator->getJointData(jointSensedVelocity);
    youBotManipulator->getJointData(jointSensedCurrent);
    youBotManipulator->getJointData(jointSensedTorque);
    youBotManipulator->getArmGripper().getGripperBar1();

    for(int i = 0; i < ARMJOINTS; ++i){
        joints_sensed_.at(i).position = jointSensedAngle.at(i).angle.value();
        joints_sensed_.at(i).velocity = jointSensedVelocity.at(i).angularVelocity.value();
        joints_sensed_.at(i).effort = jointSensedCurrent.at(i).current.value();
        joints_sensed_.at(i).torque = jointSensedTorque.at(i).torque.value();
    }
    // for(int i = ARMJOINTS; i < GRIPPERJOINTS; ++i){
    //     joints_sensed_.at(i).position = GripperSensedBarPosition.at(i-ARMJOINTS).barPosition.value();
    //     joints_sensed_.at(i).velocity = GripperSensed.at(i-ARMJOINTS).barPosition.value();
    //     joints_sensed_.at(i).position = GripperSensedBarPosition.at(i-ARMJOINTS).barPosition.value();
    //     joints_sensed_.at(i).position = GripperSensedBarPosition.at(i-ARMJOINTS).barPosition.value();
    // }

}

void YouBotArmHW::write(const ros::Time &time, const ros::Duration &period) {
    std::vector<youbot::JointCurrentSetpoint> jointsCurren(ARMJOINTS);
    std::vector<youbot::JointVelocitySetpoint> jointsVelocity(ARMJOINTS);
    std::vector<youbot::JointAngleSetpoint> jointsAngle(ARMJOINTS);
    std::vector<youbot::JointDataSetpoint> jointsData(GRIPPERJOINTS);
    static youbot::GripperBarPositionSetPoint gripper1_pose;
    static youbot::GripperBarPositionSetPoint gripper2_pose;
    static youbot::GripperBarSpacingSetPoint gripper_space;



    if(cmd_switcher_.at("youbot_arm/joints_eff_controller")){
        for(int i = 0; i < ARMJOINTS; ++i){
            jointsCurren.at(i).current = joints_cmd_.at(i).effort * ampere;
        }
        youBotManipulator->setJointData(jointsCurren);
    }
    else if(cmd_switcher_.at("youbot_arm/joints_vel_controller")){
        for(int i = 0; i < ARMJOINTS; ++i){
            jointsVelocity.at(i).angularVelocity = joints_cmd_.at(i).velocity * radian_per_second;
        }
        youBotManipulator->setJointData(jointsVelocity);
    }
    else if(cmd_switcher_.at("youbot_arm/joints_pos_controller")){
        for(int i = 0; i < ARMJOINTS; ++i){
            jointsAngle.at(i).angle = joints_cmd_.at(i).position * radian;
        }
        youBotManipulator->setJointData(jointsAngle);
    }
    if(cmd_switcher_.at("youbot_arm/gripper_pos_controller")){
        // if(joints_cmd_.at(GRIPPERBAR1).position != 0){
        gripper1_pose.barPosition = joints_cmd_.at(GRIPPERBAR1).position * meter;
        gripper2_pose.barPosition = joints_cmd_.at(GRIPPERBAR2).position * meter;
        // gripper_space.barSpacing = 0.1*meter;
        // jointsData.at(0)
        // youBotManipulator->setJointData(gripper_space);
        // youBotManipulator->getArmGripper().open();
        // }
        // else
        // {
        // // youBotManipulator->getArmGripper().close();
        // }
//        youBotArmHardware.getArmGripper().getGripperBar1().setData(gripper1_pose);
//        youBotArmHardware.getArmGripper().getGripperBar2().setData(gripper2_pose);
    //    youBotManipulator->getArmGripper().open();
    //    youBotManipulator->getArmGripper().getGripperBar2().setData(gripper2_pose);
        // gripperBarPosition.barPosition = joints_cmd_.at(GRIPPERBAR1).position * meter;
        // gripperBarPosition.barPosition = joints_cmd_.at(GRIPPERBAR2).position * meter;

        // gripperJointPositions[0].joint_uri = "gripper_finger_joint_l";
		// gripperJointPositions[0].value = joints_cmd_.at(GRIPPERBAR1).position * meter;;
		// gripperJointPositions[0].unit = boost::units::to_string(boost::units::si::meter);

        // gripperJointPositions[1].joint_uri = "gripper_finger_joint_l";
		// gripperJointPositions[1].value = joints_cmd_.at(GRIPPERBAR2).position * meter;;
		// gripperJointPositions[1].unit = boost::units::to_string(boost::units::si::meter);
        // youBotManipulator->g
        // setData(gripperJointPositions)
    }
}


}
















