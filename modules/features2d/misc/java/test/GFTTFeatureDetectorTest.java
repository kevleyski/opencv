package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;

public class GFTTFeatureDetectorTest extends OpenCVTestCase {

    public void testCreate() {
        fail("Not yet implemented");
    }

    public void testDetectListOfMatListOfListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectListOfMatListOfListOfKeyPointListOfMat() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPoint() {
        fail("Not yet implemented");
    }

    public void testDetectMatListOfKeyPointMat() {
        fail("Not yet implemented");
    }

    public void testEmpty() {
        fail("Not yet implemented");
    }

    public void testRead() {
        fail("Not yet implemented");
    }

<<<<<<< HEAD
    public void testWrite() {
        fail("Not yet implemented");
=======
    public void testWriteYml() {
        String filename = OpenCVTestRunner.getTempFileName("yml");

        detector.write(filename);

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.GFTTDetector\"\nnfeatures: 1000\nqualityLevel: 0.01\nminDistance: 1.\nblockSize: 3\ngradSize: 3\nuseHarrisDetector: 0\nk: 0.040000000000000001\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
    }

}
