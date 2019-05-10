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
#include <osg/LineWidth>

#include "Isoheight.h"

//#define OSG_PLANE_INTERSECTION
//#define TEST_POLYLINE_MAP
#define TEST_MY

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
	normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f));
	result->setNormalArray(normals, osg::Array::BIND_OVERALL);

	osg::ref_ptr<osg::StateSet> stateset = result->getOrCreateStateSet();
	osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(3.0f);
	stateset->setAttribute(lineWidth.get());

	result->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, points.size()));

	return result.release();
}

void genDebugColors(std::vector<osg::Vec4f>& colors)
{
	for (float s = 1.0f; s > 0.0f; s -= 0.4)
	{
		for (float v = 1.0f; v > 0.0f; v -= 0.3)
		{
			for (float h = 0.0f; h < 360.0f; h += 30.0f)
			{
				float c = v * s;
				float x = c * (1.0f - fabs(fmod(h / 60.0f, 2.0f) - 1.0f));
				float m = v - c;

				osg::Vec4f color;
				if (0 <= h && h < 60)
					color = osg::Vec4f(c, x, 0.0f, 1.0f);
				if (60 <= h && h < 120)
					color = osg::Vec4f(x, c, 0.0f, 1.0f);
				if (120 <= h && h < 180)
					color = osg::Vec4f(0.0f, c, x, 1.0f);
				if (180 <= h && h < 240)
					color = osg::Vec4f(0.0f, x, c, 1.0f);
				if (240 <= h && h < 300)
					color = osg::Vec4f(x, 0.0f, c, 1.0f);
				if (300 <= h && h < 360)
					color = osg::Vec4f(c, 0.0f, x, 1.0f);

				color = color + osg::Vec4f(m, m, m, 0.0f);
				colors.push_back(color);
			}
		}
	}
}

void showData(osg::Node* node)
{
	osgViewer::Viewer viewer;

	viewer.getCamera()->setClearColor(osg::Vec4(0.0f, 0.0f, 0.0f, 0.0f));
	// add the state manipulator
	viewer.addEventHandler(new osgGA::StateSetManipulator(viewer.getCamera()->getOrCreateStateSet()));

	// add the thread model handler
	viewer.addEventHandler(new osgViewer::ThreadingHandler);

	// add the window size toggle handler
	viewer.addEventHandler(new osgViewer::WindowSizeHandler);

	// add the stats handler
	viewer.addEventHandler(new osgViewer::StatsHandler);

	viewer.setSceneData(node);

	viewer.realize();
	viewer.run();
}

#ifdef OSG_PLANE_INTERSECTION 
int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		return -1;
	}
	osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(argv[1], new osgDB::Options(std::string("noRotation")));

	osg::Plane cutPlane(0.0, 0.0, 1.0, 0.01);
	osg::ref_ptr<osgUtil::PlaneIntersector> picker = new osgUtil::PlaneIntersector(cutPlane);
	osgUtil::IntersectionVisitor iv(picker.get());

	osg::Timer_t startTick = osg::Timer::instance()->tick();
	scene->accept(iv);
	osg::Timer_t endTick = osg::Timer::instance()->tick();
	std::cout << "Completed in " << osg::Timer::instance()->delta_m(startTick, endTick) << std::endl;
	
	osg::ref_ptr<osg::Geode> geode = new osg::Geode();
	if (picker->containsIntersections())
	{
		std::vector<osg::Vec4f> debugColors;
		genDebugColors(debugColors);

		osgUtil::PlaneIntersector::Intersections intersections = picker->getIntersections();
		for (std::size_t i = 0; i < intersections.size(); i++)
		{
			osgUtil::PlaneIntersector::Intersection tempIntersector = intersections[i];
			osg::ref_ptr<osg::Geometry> geom = creatPolyGeom(tempIntersector.polyline);
			
			osg::Vec4Array* colors = new osg::Vec4Array;
			if (i < debugColors.size())
			{
				colors->push_back(debugColors[i]);
			}

			geom->setColorArray(colors, osg::Array::BIND_OVERALL);
			
			geode->addDrawable(geom.get());
		}
	}
	osg::ref_ptr<osg::Group> root = new osg::Group();
	root->addChild(scene.get());
	root->addChild(geode.get());

	//osgDB::writeNodeFile(*(geode.get()), "data/line.obj");

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
#endif

#ifdef TEST_POLYLINE_MAP

typedef isoheight::PolylineConnector::PolylineMap PolylineMap;
int main(int argc, char* argv[])
{
	PolylineMap test;
	
	osg::Vec4d p1(1.0, 1.0, 1.0, 0.0);
	isoheight::RefPolyline* polyline1 = new isoheight::RefPolyline;
	test[p1] = polyline1;

	osg::Vec4d p2(1.15, 1.0, 1.0, 0.0);
	isoheight::RefPolyline* polyline2 = new isoheight::RefPolyline;
	test[p2] = polyline2;

	osg::Vec4d p3(1.08, 1.0, 1.0, 0.0);
	isoheight::RefPolyline* polyline3 = new isoheight::RefPolyline;
	test[p3] = polyline3;

	PolylineMap::iterator pos = test.find(osg::Vec4d(1.11, 1.0, 1.0, 0.0));

	if (pos != test.end())
	{
		std::cout << pos->first;
	}



	return 0;

}

#endif

#ifdef TEST_MY

#include <OpenMesh/Core/IO/MeshIO.hh>
#include <OpenMesh/Core/Mesh/TriMesh_ArrayKernelT.hh>

struct MeshTraits : public OpenMesh::DefaultTraits
{
	typedef OpenMesh::Vec3d Point;

};
typedef OpenMesh::TriMesh_ArrayKernelT<MeshTraits> MyMesh;

osg::Vec3d& convVecType(const MeshTraits::Point& omPoint)
{
	return osg::Vec3d(omPoint[0], omPoint[1], omPoint[2]);
}


int main(int argc, char* argv[])
{
	MyMesh triMesh;
	bool readOk =  OpenMesh::IO::read_mesh(triMesh, argv[1]);
	if (!readOk)
	{
		std::cout << "read mesh fail" << std::endl;
		return -1;
	}

	osg::ref_ptr<osg::Group> root = new osg::Group();

	osg::ref_ptr<osg::Node> scene = osgDB::readNodeFile(argv[1], new osgDB::Options(std::string("noRotation")));
	root->addChild(scene.get());

	std::vector<osg::Vec4f> debugColors;
	genDebugColors(debugColors);

	for (std::size_t k = 0; k < 100; k++)
	{
		isoheight::TriangleIntersector triIntersect;
		triIntersect.set(osg::Plane(0.0, 0.0, -1.0, -50.0 + k * 5));

		MyMesh::FaceIter f_it = triMesh.faces_begin();
		for (; f_it != triMesh.faces_end(); f_it++)
		{
			MyMesh::HalfedgeHandle heh1 = triMesh.halfedge_handle(*f_it);
			MyMesh::HalfedgeHandle heh2 = triMesh.next_halfedge_handle(heh1);
			MyMesh::HalfedgeHandle heh3 = triMesh.next_halfedge_handle(heh2);

			MeshTraits::Point p1 = triMesh.point(triMesh.to_vertex_handle(heh1));
			MeshTraits::Point p2 = triMesh.point(triMesh.to_vertex_handle(heh2));
			MeshTraits::Point p3 = triMesh.point(triMesh.to_vertex_handle(heh3));

			osg::Vec3d v1 = convVecType(p1);
			osg::Vec3d v2 = convVecType(p2);
			osg::Vec3d v3 = convVecType(p3);

			triIntersect(v1, v2, v3, true);
		}

		triIntersect._polylineConnector.consolidatePolylineLists();

		osg::ref_ptr<osg::Geode> geode = new osg::Geode();
		for (std::size_t i = 0; i < triIntersect._polylineConnector._polylines.size(); i++)
		{
			osg::ref_ptr<isoheight::RefPolyline> polyLine = triIntersect._polylineConnector._polylines[i];

			std::vector<osg::Vec3d> points;
			for (std::size_t j = 0; j < polyLine->_polyline.size(); j++)
			{
				points.push_back(osg::Vec3d(polyLine->_polyline[j][0], polyLine->_polyline[j][1], polyLine->_polyline[j][2]));
			}

			osg::ref_ptr<osg::Geometry> geom = creatPolyGeom(points);
			osg::Vec4Array* colors = new osg::Vec4Array;

			colors->push_back(debugColors[k % debugColors.size()]);
	

			geom->setColorArray(colors, osg::Array::BIND_OVERALL);
			geode->addDrawable(geom.get());
		}

		root->addChild(geode.get());
	}

	showData(root.get());
	

	return 0;
}




#endif

