/**
 @author Karsten Knese
 @author Alessio Rocchi
 */

#include <OpenSoT/constraints/velocity/SelfCollisionAvoidance.h>
#include <geometry_msgs/Transform.h>
#include <idynutils/collision_utils.h>
#include <ros/ros.h>
#include <sensor_msgs/JointState.h>
#include <tf/transform_broadcaster.h>
#include <visualization_msgs/Marker.h>
#include <visualization_msgs/MarkerArray.h>

static const std::string JOINT_STATE_TOPIC = "/joint_states";
static const std::string RESULT_MARKER_TOPIC = "distance_query/result_marker";

int id_counter = 0;
int id_lines = 0;
std::string base_frame = "base_link";

const int kelly_colors_hex_size = 20;
const int kelly_colors_hex[] = {
    0xFFB300, // Vivid Yellow
    0x803E75, // Strong Purple
    0xFF6800, // Vivid Orange
    0xA6BDD7, // Very Light Blue
    0xC10020, // Vivid Red
    0xCEA262, // Grayish Yellow
    0x817066, // Medium Gray

    // The following don't work well for people with defective color vision
    0x007D34, // Vivid Green
    0xF6768E, // Strong Purplish Pink
    0x00538A, // Strong Blue
    0xFF7A5C, // Strong Yellowish Pink
    0x53377A, // Strong Violet
    0xFF8E00, // Vivid Orange Yellow
    0xB32851, // Strong Purplish Red
    0xF4C800, // Vivid Greenish Yellow
    0x7F180D, // Strong Reddish Brown
    0x93AA00, // Vivid Yellowish Green
    0x593315, // Deep Yellowish Brown
    0xF13A13, // Vivid Reddish Orange
    0x232C16  // Dark Olive Green
};

const unsigned int GREY =   0x817066;
const unsigned int RED =    0xC10020;
const unsigned int PURPLE = 0x803E75;
const unsigned int GREEN =  0x007D34;
const unsigned int YELLOW = 0xFFB300;
const unsigned int BLUE =   0x00538A;

bool draw_point(const double x, const double y, const double z,
                const std::string frame, visualization_msgs::Marker& marker, unsigned int color=0xff0000) {
    //DRAW REFERENCE
    marker.header.frame_id = frame;
    marker.header.stamp = ros::Time().now();
    marker.ns = "goal";
    marker.id = ++id_counter;
    marker.type = visualization_msgs::Marker::SPHERE;
    marker.action = visualization_msgs::Marker::ADD;

    marker.pose.position.x = x;
    marker.pose.position.y = y;
    marker.pose.position.z = z;
    marker.pose.orientation.x = 0.0;
    marker.pose.orientation.y = 0.0;
    marker.pose.orientation.z = 0.0;
    marker.pose.orientation.w = 1.0;
    marker.scale.x = 0.02;
    marker.scale.y = 0.02;
    marker.scale.z = 0.02;

    /* // use the map of kelly_colors
    color = kelly_colors_hex[id_lines];*/
    marker.color.r = (float)(((color & 0xff0000) >> 16)/255.0);
    marker.color.g = (float)(((color & 0x00ff00) >> 8 )/255.0);
    marker.color.b = (float)(((color & 0x0000ff)      )/255.0);

    marker.color.a = 1.0;
    return true;
}

bool draw_line(const double x1, const double y1, const double z1,
               const double x2, const double y2, const double z2,
               visualization_msgs::Marker& marker, unsigned int color=0xff0000) {
    //DRAW REFERENCE
    marker.header.frame_id = "Waist";
    marker.header.stamp = ros::Time().now();
    marker.ns = "goal";
    marker.id = ++id_counter;
    marker.type = visualization_msgs::Marker::LINE_STRIP;
    marker.action = visualization_msgs::Marker::ADD;

    geometry_msgs::Point p1; p1.x = x1; p1.y = y1; p1.z = z1;
    geometry_msgs::Point p2; p2.x = x2; p2.y = y2; p2.z = z2;
    marker.points.push_back(p1);
    marker.points.push_back(p2);

    marker.scale.x = 0.005;
    marker.scale.y = 0.005;
    marker.scale.z = 0.005;

    /* // use the map of kelly_colors
    color = kelly_colors_hex[id_lines];*/
    marker.color.r = (float)(((color & 0xff0000) >> 16)/255.0);
    marker.color.g = (float)(((color & 0x00ff00) >> 8 )/255.0);
    marker.color.b = (float)(((color & 0x0000ff)      )/255.0);

    marker.color.a = 1.0;

    id_lines = (id_lines+1)% kelly_colors_hex_size; // can be used in the future to index the kelly_colors array
    return true;
}

void createMarkerArray(std::list<LinkPairDistance>& results,
                       const boost::shared_ptr<visualization_msgs::MarkerArray>& markers,
                       iDynUtils& model,
                       unsigned int markerColor=0xff0000,
                       unsigned int lineColor=0xff0000) {
    typedef std::list<LinkPairDistance>::iterator iter_pairs;
    for(iter_pairs it = results.begin(); it != results.end(); ++it)
    {
        visualization_msgs::Marker m1;
        std::pair<std::string, std::string> linkNames = it->getLinkNames();
        std::pair<KDL::Frame, KDL::Frame> transforms = it->getLink_T_closestPoint();
        draw_point( transforms.first.p.x(),
                    transforms.first.p.y(),
                    transforms.first.p.z(),
                    linkNames.first, m1, markerColor);


        visualization_msgs::Marker m2;
        draw_point( transforms.second.p.x(),
                    transforms.second.p.y(),
                    transforms.second.p.z(),
                    linkNames.second, m2, markerColor);

        visualization_msgs::Marker l12;
        KDL::Frame p1 = model.iDyn3_model.getPositionKDL(
                            model.iDyn3_model.getLinkIndex("Waist"),
                            model.iDyn3_model.getLinkIndex(linkNames.first))*transforms.first;
        KDL::Frame p2 = model.iDyn3_model.getPositionKDL(
                            model.iDyn3_model.getLinkIndex("Waist"),
                            model.iDyn3_model.getLinkIndex(linkNames.second))*transforms.second;
        draw_line(  p1.p.x(), p1.p.y(), p1.p.z(),
                    p2.p.x(), p2.p.y(), p2.p.z(),
                    l12, lineColor);

        markers->markers.push_back(m1);
        markers->markers.push_back(m2);
        markers->markers.push_back(l12);
    }
}

int main(int argc, char** argv) {
    iDynUtils bigman("bigman",
                     std::string(OPENSOT_TESTS_ROBOTS_DIR)+"bigman/bigman.urdf",
                     std::string(OPENSOT_TESTS_ROBOTS_DIR)+"bigman/bigman.srdf");

    ros::init(argc, argv, "distance_computation");
    ros::NodeHandle nh;
    double rate = 100;
    ros::Rate loopRate(rate);

    const sensor_msgs::JointStateConstPtr initJoints =
            ros::topic::waitForMessage<sensor_msgs::JointState>(
                    JOINT_STATE_TOPIC, nh);

    OpenSoT::constraints::velocity::SelfCollisionAvoidance::Ptr sca(
        new OpenSoT::constraints::velocity::SelfCollisionAvoidance(
            bigman.iDyn3_model.getAng(),
            bigman, 0.005, 0.3));

    boost::shared_ptr<ComputeLinksDistance> distance_comp(
            new ComputeLinksDistance(bigman));

    ros::Subscriber joint_states_subscriber = nh.subscribe<
            sensor_msgs::JointState>(JOINT_STATE_TOPIC, 1, // Buffer size
            &iDynUtils::updateiDyn3ModelFromJoinStateMsg, &bigman);

    ros::Publisher resultMarkerPub = nh.advertise<
            visualization_msgs::MarkerArray>(RESULT_MARKER_TOPIC, 10);
    boost::shared_ptr<visualization_msgs::MarkerArray> markers(
            new visualization_msgs::MarkerArray);

    yarp::sig::Vector q_curr = bigman.iDyn3_model.getAng();
    while (ros::ok()) {

        q_curr = bigman.iDyn3_model.getAng();

        sca->predict_SCAFoIs(q_curr,
                             sca->Linkpair_updated_list_all,
                             sca->Linkpair_constrained_list_all);

        std::list<LinkPairDistance>
            results_in_scafois, // all link pairs which are in an activated SCAFoI
            results_constrained; // all link pairs not in an active SCAFoI, with distance > d_threshold_upper

        distance_comp->setCollisionWhiteList(sca->Linkpair_updated_list_all);
        std::list<LinkPairDistance> results = distance_comp->getLinkDistances();

        if (results.size() > 0) {
            markers->markers.clear();
            id_counter = 0;
            id_lines = 0;

            for(std::list<LinkPairDistance>::iterator it =
                    results.begin();
                it != results.end(); ++it)
            {
                if(it->getDistance() < sca->d_threshold_lower)
                    results_constrained.push_back(*it);
                else
                    results_in_scafois.push_back(*it);
            }

            if(results_in_scafois.size() > 0)
                createMarkerArray(results_in_scafois, markers, bigman,  RED,     YELLOW);

            if(results_constrained.size() > 0)
                createMarkerArray(results_constrained, markers, bigman,  RED,    RED);

            resultMarkerPub.publish(markers);
        }

        ros::spinOnce();
        loopRate.sleep();
    }

    return EXIT_SUCCESS;
}
