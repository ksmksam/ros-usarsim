/*****************************************************************************
 DISCLAIMER:
 This software was produced by the National Institute of Standards
 and Technology (NIST), an agency of the U.S. government, and by statute is
 not subject to copyright in the United States.  Recipients of this software
 assume all responsibility associated with its operation, modification,
 maintenance, and subsequent redistribution.
 
 See NIST Administration Manual 4.09.07 b and Appendix I.
 *****************************************************************************/

#include "MatrixOperator.h"
#include <math.h>

/**
 \brief Constructor.
 
 Set the file operator @a op to MatrixOperator::m_file_operator.
 */
MatrixOperator::MatrixOperator(FileOperator *op)
{
    setFileOperator(op);
}

/**
 \brief Constructor.
 */
MatrixOperator::MatrixOperator()
{
}

/**
 \brief Destructor.
 */
MatrixOperator::~MatrixOperator()
{
}

/**
 \brief Compute the position of each link in FileOperator::m_armlink based on the position and orientation of each joint in FileOperator::m_armjoint.
 
 <ul>
 <li> For each joint in FileOperator::m_armjoint:
 <ol>
 <li> Get the position (xyz) \f$ j^A \f$ of the joint defined in the current reference frame \f$ C \f$.
 <li> Get the orientation (rpy) of the joint from the original reference frame \f$ A \f$ to the current reference frame \f$ C \f$.
 </ol>
 </ul>
 For each joint in FileOperator::m_armjoint, retrieve the child link and compute the position of this link in the joint reference frame.
 */
void MatrixOperator::computeLinkPosition()
{
    std::vector<ArmJoint*> v_armjoint = m_file_operator->getArmJoint();
    int v_armjoint_size = (int)v_armjoint.size();
    std::vector<Vector3d> link_position_set;
    
    for (vector<ArmJoint*>::size_type i=1; i<v_armjoint_size; i++)
    {
        std::string child = v_armjoint[i]->getJointChild();
        
        std::vector<double> v_current_joint_offset = v_armjoint[i]->getJointOffsetVector();
        std::vector<double> v_tmpRPY = v_armjoint[i]->getJointRPYVector();
        
        Matrix3d matrix = buildRotationMatrix(v_tmpRPY);
        
        Matrix4d m4d;
        m4d.block(0,0,3,3) << matrix;
        m4d.col(3) <<
        v_current_joint_offset[0],
        v_current_joint_offset[1],
        v_current_joint_offset[2],1;
        
        m4d.row(3)<< 0, 0, 0, 1;
        
        std::vector<ArmLink*> v_armlink = m_file_operator->getArmLink();
        int v_armlink_size = (int)v_armlink.size();
        
        for (vector<ArmLink*>::size_type j=0; j<v_armlink_size; j++)
        {
            std::vector<double> v_current_link_offset = v_armlink[j]->getOffsetVector();
            std::string s_link_name = v_armlink[j]->getLinkName();
            
            if (s_link_name.compare(child)==0)
            {
                Vector4d v_linkPosition(v_current_link_offset[0],
                                        v_current_link_offset[1],
                                        v_current_link_offset[2],
                                        1);
                
                Vector3d _position;
                
                Vector4d final_position;
                final_position=m4d.inverse()*v_linkPosition;
                
                double scale = 0.001;  // i.e. round to nearest one-hundreth
                _position[0]=floor(final_position[0] / scale + 0.5) * scale;
                _position[1]=floor(final_position[1] / scale + 0.5) * scale;
                _position[2]=floor(final_position[2] / scale + 0.5) * scale;
                
                //-- Update the current link positions in m_linkOffsetVector with the ones just computed
                v_armlink[j]->setOffsetVector((double)_position[0],(double)_position[1],(double)_position[2]);
                
                //-- Write this Vector3d in the vector link_position_set
                link_position_set.push_back(_position);
            }
        }
    }
    //-- Set the vector<Vector3d>
    m_file_operator->setLinkPosition(link_position_set);
}


void MatrixOperator::computeLinkOrientation() {}

/*!
 \brief Compute the position of each joint in FileOperator::m_armJoint.
 
 This function reads FileOperator::m_armJoint and computes the position of each joint based on the position of the previous joint in FileOperator::m_armJoint. Only the position of the first joint is not computed since it is the base joint that is used to compute the position of the following joint.
 
 The different steps to compute the position of each joint are:
 <ol>
 <li> Retrieve each joint in FileOperator::m_armJoint starting from the second joint.
 <li>For each joint:
 <ul>
 <li>Get the associated offset where each offset indicates the relative position of the joint to the first link in the .uc script.
 <li>Compute the rotation matrix between two following joints in FileOperator::m_armJoint.
 <li>Compute the homogeneous matrix using the rotation matrix and the joint offset.
 <li>Compute the position of the joint in the new reference frame.
 </ul>
 </ol>
 
 */
void MatrixOperator::computeJointPosition()
{
    std::vector<ArmJoint*> v_armjoint = m_file_operator->getArmJoint();
    int v_armjoint_size = (int)v_armjoint.size();
    std::vector<Vector3d> joint_position_set;
    
    for (vector<ArmJoint*>::size_type i = 0; i < v_armjoint_size; i++)
    {
        std::vector<double> v_current_tmpOffset = v_armjoint[i]->getJointOffsetVector();
        
        if (i>0)
        {
            std::vector<double> v_prev_tmpOffset = v_armjoint[i-1]->getJointOffsetVector();
            std::vector<double> v_tmpRPY = v_armjoint[i-1]->getJointRPYVector();
            Matrix3d matrix = buildRotationMatrix(v_tmpRPY);
            
            Matrix4d m4d;
            m4d.block(0,0,3,3) << matrix;
            m4d.col(3) << v_prev_tmpOffset[0], v_prev_tmpOffset[1], v_prev_tmpOffset[2],1;
            m4d.row(3)<< 0, 0, 0, 1;
            
            Vector4d v_jointPosition(v_current_tmpOffset[0],v_current_tmpOffset[1],v_current_tmpOffset[2],1);
            Vector3d _position;
            
            Vector4d final_position;
            final_position=m4d.inverse()*v_jointPosition;
            
            double scale = 0.001;  // i.e. round to nearest one-hundreth
            _position[0]=floor(final_position[0] / scale + 0.5) * scale;
            _position[1]=floor(final_position[1] / scale + 0.5) * scale;
            _position[2]=floor(final_position[2] / scale + 0.5) * scale;
            
            //-- Update the offset for each joint with the ones just computed
            v_armjoint[i]->setJointOffsetVector((double)_position[0],(double)_position[1],(double)_position[2]);
            
            joint_position_set.push_back(_position);
        }
    }
    m_file_operator->setJointPosition(joint_position_set);
}

Matrix3d MatrixOperator::buildRotationMatrix(std::vector<double> _rpy_vector)
{
    Matrix3d matrix;
    
    
    //std::cout << m1.format(CleanFmt) << sep;
    
    if (_rpy_vector.size()==3)
    {
        //cout << "rpy: "<<_rpy_vector[0] <<", "<<_rpy_vector[1]<<", "<<_rpy_vector[02]<<endl;
        
        matrix = AngleAxisd(_rpy_vector[2], Vector3d::UnitZ())
        * AngleAxisd(_rpy_vector[1], Vector3d::UnitY())
        * AngleAxisd(_rpy_vector[0], Vector3d::UnitX());
        
        //matrixf = AngleAxisf(-0.5*(M_PI), Vector3f::UnitX());
    }
    return matrix;
}

/**
 \brief Truncate a number by defining the number of decimals.
 
 \param value The value to truncate
 \param decimal The number of decimals in the result
 */
double MatrixOperator::truncate(double value, double decimal){
    double f = pow(10.0,decimal);
    return round(value*f)/f;
}

/**
 \brief Compute the joint angles for a joint relative to the reference frame of the parent link.
 
 In the .uc file, joint angles are relative to the orientation of the first link placed in the environment.<br>
 In ROS, joint angles are relative to the reference frame of its parent link.<br><br>
 
 In the Figure below Joint2 is offset in the \f$y\f$-direction from link1, a little offset in the negative \f$x\f$-direction from link1, and it is rotated 90 degrees around the \f$z\f$-axis.
 <br>Therefore, the joint angles for Joint2 is described as rpy="0 0 1.57".
 \image html link2.png
 \image latex link.png "Links and Joints" width=10cm
 
 To describe how this function computes joit angles for a joint relative to its parent link, let's go back to the picture depicted above and use Joint3 as an example.<br>
 Let:
 <ul>
 <li>\f$ R^3_1\f$ the rotation matrix from Link1 to Joint3: The rpy for this joint is given in the .uc file.
 <li>\f$ R^2_1\f$ the rotation matrix from Link1 to Joint2: The rpy for this joint is given in the .uc file.
 <li>\f$ R^3_2\f$ the rotation matrix from Joint2 to Joint3: This rotation matrix is computed using \f$ R^3_1\f$ and \f$ R^2_1\f$.
 </ul>
 We have: \f[ R^3_1 = R^2_1 R^3_2 \Rightarrow R^3_2 = (R^2_1)^{-1} R^3_1\f]
 
 Once \f$R^3_2\f$ is computed, the function MatrixOperator::getAngles(Matrix3d) is used to compute the joint angles for a joint in the new reference frame.
 
 
 */
void MatrixOperator::computeJointAngles()
{
    std::vector<ArmJoint*> v_armjoint = m_file_operator->getArmJoint();
    int v_armjoint_size = (int)v_armjoint.size();
    std::vector<Vector3d> v_joint_angle;
    Matrix3d r12, r13, r23;
    
    IOFormat CleanFmt(3, 0, ", ", "\n", "[", "]");
    //std::string sep = "\n----------------------------------------\n";
    std::string sep2 = "\n\n*******************************************************\n";
    
    for (vector<ArmJoint*>::size_type i = 0; i < v_armjoint_size; i++)
    {
        std::string jointname = v_armjoint[i]->getJointName();
        std::vector<double> current_joint_rpy = v_armjoint[i]->getJointRPYVector();
        
        //cout << "\n---> Current Joint: "<<jointname<<" : " << current_joint_rpy[0]<< ","<<current_joint_rpy[1]<< ","<<current_joint_rpy[2]<< endl;
        
        //-- Starting from the third joint in the list
        if (i>1)
        {
            //-- Compute the rotation matrix from the original (first) joint to the previous joint: R^2_1
            std::string prev_jointname = v_armjoint[i-1]->getJointName();
            std::vector<double> previous_joint_rpy = v_armjoint[i-1]->getJointRPYVector();
            
            //cout << "\n---> Previous Joint: "<<prev_jointname<<" : " << previous_joint_rpy[0]<< ","<<previous_joint_rpy[1]<< ","<<previous_joint_rpy[2]<< endl;
            //cout << sep2;
            //cout << "\n---> Angles from : "<< prev_jointname <<" to " << jointname<<" : " << endl;
            
            r12 = AngleAxisd(previous_joint_rpy[2], Vector3d::UnitZ())
            * AngleAxisd(previous_joint_rpy[1], Vector3d::UnitY())
            * AngleAxisd(previous_joint_rpy[0], Vector3d::UnitX());
            
            //-- Compute the rotation matrix from the original (first) joint to the current joint: R^3_1
            r13 = AngleAxisd(current_joint_rpy[2], Vector3d::UnitZ())
            * AngleAxisd(current_joint_rpy[1], Vector3d::UnitY())
            * AngleAxisd(current_joint_rpy[0], Vector3d::UnitX());
            
            //-- Compute the rotation matrix from the previous joint to the current joint: R^3_2
            r23 = r12.inverse()*r13;
            
            //cout << r23.format(CleanFmt) << sep;
            Vector3d v = getAnglesFromRotationMatrix(r23);
            
            //-- Update joint rpy
            v_armjoint[i]->setJointRPYVector((double)v[0],(double)v[1],(double)v[2]);
            
            v_joint_angle.push_back(v);
        }
        //-- For the first 2 joints, the joint angles
        else {
            Vector3d v(current_joint_rpy[0], current_joint_rpy[1], current_joint_rpy[2]);
            v_joint_angle.push_back(v);
        }
    }
    m_file_operator->setJointAngle(v_joint_angle);
}

/**
 This function describes a simple technique to find all possible Euler angles from
 a rotation matrix.
 
 <h2>Rotation matrices</h2>
 We start off with the standard definition of the rotations about the three principle axes.<br>
 
 A rotation of \f$ \psi \f$ radians about the \f$ x\f$-axis is defined as
 
 \f[
 R_x(\psi) = {\left[
 \begin{array}{ccc}
 1 & 0 & 0\\
 0 &\cos{\psi} & -\sin{\psi}\\
 0 &\sin{\psi} &\cos{\psi}
 \end{array}\right]}
 \f]
 
 Similarly, a rotation of \f$ \theta \f$ radians about the \f$ y\f$-axis is defined as
 
 \f[
 R_y(\theta) = {\left[
 \begin{array}{ccc}
 \cos(\theta) & 0 & \sin{\theta} \\
 0 & 1 & 0 \\
 -\sin{\theta} & 0 & \cos{\theta}
 \end{array}
 \right]}
 \f]
 
 Finally, a rotation of \f$ \phi \f$ radians about the \f$ z\f$-axis is defined as
 
 \f[
 R_z(\phi) = {\left[
 \begin{array}{ccc}
 \cos(\phi) & -\sin{\phi} & 0 \\
 \sin{\phi} & \cos(\phi) & 0 \\
 0 & 0 & 1
 \end{array}
 \right]}
 \f]
 
 The angles \f$ \psi \f$, \f$ \theta \f$, and \f$ \phi \f$ are the Euler angles.
 
 <h2>Generalized Rotation Matrices</h2>
 
 A general rotation matrix can will have the form,
 \f[
 R = {\left[
 \begin{array}{ccc}
 R_{11} & R_{12} & R_{13} \\
 R_{21} & R_{22} & R_{23} \\
 R_{31} & R_{32} & R_{33}
 \end{array}\right]}
 \f]
 
 This matrix can be thought of a sequence of three rotations, one about each
 principle axis. Since matrix multiplication does not commute, the order of the
 axes which one rotates about will affect the result. For this analysis, we will
 rotate first about the \f$ x\f$-axis, then the \f$ y\f$-axis, and finally the \f$ z\f$-axis.
 
 <h2>C++ Pseudo Code to Compute Euler Angles from a Rotation Matrix</h2>
 The C++ pseudo code for this function is given in the Figure below:<br><br>
 \image html getAnglesFromRotationMatrix.png
 
 <h2>More than one Solution? </h2>
 It is interesting to note that there is always more than one sequence of rotations
 about the three principle axes that results in the same orientation of an object.
 As we have shown in the above explanation, in the non-degenerate case of cos θ = 0, there
 are two solutions. For the degenerate case of cos θ = 0, an infinite number of
 solutions exist.
 
 \note The rows and columns for the rotation matrix \f$R\f$ defined above start at index 1 while matrices in the Eigen library start at index 0. Using the elements described above for the
 rotation matrix \f$R\f$, we give the corresponding C++ elements for the Eigen matrix @a mat3d:
 <ul>
 <li> \f$R_{11}\f$=@a mat3d(0,0)
 <li> \f$R_{12}\f$=@a mat3d(0,1)
 <li> \f$R_{13}\f$=@a mat3d(0,2)
 <li> \f$R_{21}\f$=@a mat3d(1,0)
 <li> \f$R_{31}\f$=@a mat3d(2,0)
 <li> \f$R_{32}\f$=@a mat3d(2,1)
 <li> \f$R_{33}\f$=@a mat3d(2,2)
 </ul>
 
 \param mat3d A rotation matrix
 */
Vector3d MatrixOperator::getAnglesFromRotationMatrix(Matrix3d mat3d)
{
    double theta_1, theta_2, psi_1, psi_2, phi_1, phi_2;
    Vector3d rpy;
    
    if (mat3d(2,0)<1 || mat3d(2,0)>1)
    {
        //-- Compute theta angles
        theta_1 = truncate(-asin(mat3d(2,0)),3);
        theta_2 = truncate(M_PI-theta_1,3);
        
        //-- Compute psi angles
        psi_1 = truncate(atan2(mat3d(2,1)/cos(theta_1),mat3d(2,2)/cos(theta_1)),3);
        psi_2 = truncate(atan2(mat3d(2,1)/cos(theta_2),mat3d(2,2)/cos(theta_2)),3);
        
        //--Compute phi angles
        phi_1 = truncate(atan2(mat3d(1,0)/cos(theta_1),mat3d(0,0)/cos(theta_1)),3);
        phi_2 = truncate(atan2(mat3d(1,0)/cos(theta_2),mat3d(0,0)/cos(theta_2)),3);
        
        //cout << "2 Solutions" << endl;
        //cout << "psi_1: " << psi_1 << ", psi_2: " << psi_2 << endl;
        //cout << "theta_1: " << theta_1 << ", theta_2: " << theta_2 << endl;
        //cout << "phi_1: " << phi_1 << ", phi_2: " << phi_2 << endl;
    }
    else {
        phi_1=0;
        if (mat3d(2,0)==-1){
            theta_1=truncate(0.5*M_PI,3);
            psi_1=truncate(phi_1+atan2(mat3d(0,1),mat3d(0,2)),3);
        }
        else{
            theta_1=truncate(-0.5*M_PI,3);
            psi_1=truncate(-phi_1+atan2(-mat3d(0,1),-mat3d(0,2)),3);
        }
        //cout << "psi_1: " << psi_1 << endl;
        //cout << "theta_1: " << theta_1 << endl;
        //cout << "phi_1: " << phi_1 << endl;
    }
    
    rpy (0)=psi_1;
    rpy (1)=theta_1;
    rpy (2)=phi_1;
    
    return rpy;
}

/*!
 \brief Set \a op to MatrixOperator::m_file_operator.
 \param op A FileOperator object.
 */
void MatrixOperator::setFileOperator(FileOperator *op)
{
    m_file_operator=op;
}

/*!
 \brief Set \a op to MatrixOperator::m_file_operator.
 \param op A FileOperator object.
 */
FileOperator* MatrixOperator::getFileOperator()
{
    return m_file_operator;
}