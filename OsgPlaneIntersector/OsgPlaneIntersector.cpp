#ifdef _WIN32
#include <Windows.h>
#endif // _WIN32

//viewer
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
//db
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
//util
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/PlaneIntersector>
//ga
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
//node
#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Group>
#include <osg/Timer>

osg::Geometry* creatPolyGeom(const std::vector<osg::Vec3d>& points)
{
	osg::ref_ptr<osg::Geometry> result = new osg::Geometry();

	osg::ref_ptr<osg::Vec3dArray> vertexs = new osg::Vec3dArray();

	for (std::size_t i = 0; i < points.size(); i++)
	{
		vertexs->push_back(points[i]);
	}

	result->setVertexArray(vertexs.get());


	osg::Vec4Array* colors = new osg::Vec4Array;
	colors->push_back(osg::Vec4(1.0f, 1.0f, 0.0f, 1.0f));
	result->setColorArray(colors, osg::Array::BIND_OVERALL);

	osg::Vec3Array* normals = new osg::Vec3Array;
	normals->push_back(osg::Vec3(0.0f, -1.0f, 0.0f));
	result->setNormalArray(normals, osg::Array::BIND_OVERALL);

	result->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, points.size()));

	return result.release();
}


int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		return -1;
	}
	osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(argv[1]);

	osg::Plane cutPlane(0.0, 0.0, 1.0, 0.0);
	osg::ref_ptr<osgUtil::PlaneIntersector> picker = new osgUtil::PlaneIntersector(cutPlane);
	osgUtil::IntersectionVisitor iv(picker.get());

	osg::Timer_t startTick = osg::Timer::instance()->tick();
	scene->accept(iv);
	osg::Timer_t endTick = osg::Timer::instance()->tick();
	std::cout << "Completed in " << osg::Timer::instance()->delta_m(startTick, endTick) << std::endl;
	
	osg::ref_ptr<osg::Geode> geode = new osg::Geode();
	if (picker->containsIntersections())
	{
		osgUtil::PlaneIntersector::Intersections intersections = picker->getIntersections();

		for (std::size_t i = 0; i < intersections.size(); i++)
		{
			osgUtil::PlaneIntersector::Intersection tempIntersector = intersections[i];
			osg::ref_ptr<osg::Geometry> geom = creatPolyGeom(tempIntersector.polyline);
			geode->addDrawable(geom.get());
		}
	}
	osg::ref_ptr<osg::Group> root = new osg::Group();
	root->addChild(scene.get());
	root->addChild(geode.get());

	osgViewer::Viewer viewer;
	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

	// add the thread model handler
	viewer.addEventHandler(new osgViewer::ThreadingHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	viewer.setSceneData(root.get());

	viewer.realize();
	viewer.run();


	return 0;
}