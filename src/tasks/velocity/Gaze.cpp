#include <OpenSoT/tasks/velocity/Gaze.h>


using namespace OpenSoT::tasks::velocity;


Gaze::Gaze(std::string task_id,
           const Eigen::VectorXd& x,
           XBot::ModelInterface &robot,
           std::string base_link) :
    Task(task_id, x.size()),
    _distal_link("gaze"),
    _cartesian_task(new Cartesian(task_id, x, robot, _distal_link, base_link)),
    _subtask(new SubTask(_cartesian_task, Indices::range(4,5))),
    _robot(robot), _gaze_T_obj(4,4), _tmp_vector(3), _bl_T_gaze_kdl(),
    _gaze_goal(), _tmpEigenM(4,4), _tmpEigenM2(4,4)
{
    this->_update(x);
}

Gaze::~Gaze()
{

}

void Gaze::setGaze(const KDL::Frame& desiredGaze)
{
    _tmpEigenM2.setIdentity(4,4);
    //Here we just need the position part
    _tmpEigenM2(0,3) = desiredGaze.p.x();
    _tmpEigenM2(1,3) = desiredGaze.p.y();
    _tmpEigenM2(2,3) = desiredGaze.p.z();
    setGaze(_tmpEigenM2);
}

void Gaze::setGaze(const Eigen::MatrixXd &desiredGaze)
{    
    _tmpEigenM.setIdentity(4,4);

    if(_cartesian_task->baseLinkIsWorld())
        _robot.getPose(_distal_link, _bl_T_gaze_kdl);
    else
        _robot.getPose(_distal_link, _cartesian_task->getBaseLink(), _bl_T_gaze_kdl);

    for(unsigned int i = 0; i < 3; ++i){
        for(unsigned int j = 0; j < 3; ++j)
            _tmpEigenM(i,j) = _bl_T_gaze_kdl.M(i,j);
    }
    _tmpEigenM(0,3) = _bl_T_gaze_kdl.p.x();
    _tmpEigenM(1,3) = _bl_T_gaze_kdl.p.y();
    _tmpEigenM(2,3) = _bl_T_gaze_kdl.p.z();


    _gaze_T_obj = _tmpEigenM.inverse()*desiredGaze;
    _tmp_vector(0) = _gaze_T_obj(0,3);
    _tmp_vector(1) = _gaze_T_obj(1,3);
    _tmp_vector(2) = _gaze_T_obj(2,3);

    _gaze_goal = _gaze_goal.Identity();
    cartesian_utils::computePanTiltMatrix(_tmp_vector, _gaze_goal);
    //cartesian_utils::computePanTiltMatrix(gaze_T_obj.subcol(0, 3, 3), gaze_goal);

    _cartesian_task->setReference(_bl_T_gaze_kdl*_gaze_goal);
}

void Gaze::setOrientationErrorGain(const double& orientationErrorGain)
{
    _cartesian_task->setOrientationErrorGain(orientationErrorGain);
}

const double Gaze::getOrientationErrorGain() const
{
    return _cartesian_task->getOrientationErrorGain();
}

void Gaze::setWeight(const Eigen::MatrixXd &W)
{
    this->_W = W;
    _subtask->setWeight(W);
}

std::list<OpenSoT::SubTask::ConstraintPtr> &Gaze::getConstraints()
{
    return _subtask->getConstraints();
}

const unsigned int Gaze::getTaskSize() const
{
    return _subtask->getTaskSize();
}

void Gaze::_update(const Eigen::VectorXd &x)
{
    _subtask->update(x);
    this->_A = _subtask->getA();
    this->_b = _subtask->getb();
    this->_hessianType = _subtask->getHessianAtype();
    this->_W = _subtask->getWeight();
    
    setA(_A);
    setb(_b);
    setHessianType(_hessianType);
    setWeight(_W);
}

std::vector<bool> Gaze::getActiveJointsMask()
{
    return _subtask->getActiveJointsMask();
}

bool Gaze::setActiveJointsMask(const std::vector<bool> &active_joints_mask)
{
    return _subtask->setActiveJointsMask(active_joints_mask);
}
