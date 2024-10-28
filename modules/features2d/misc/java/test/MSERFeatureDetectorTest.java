package org.opencv.test.features2d;

import org.opencv.test.OpenCVTestCase;

public class MSERFeatureDetectorTest extends OpenCVTestCase {

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

        String truth = "%YAML:1.0\n---\nname: \"Feature2D.MSER\"\ndelta: 5\nminArea: 60\nmaxArea: 14400\nmaxVariation: 0.25\nminDiversity: 0.20000000000000001\nmaxEvolution: 200\nareaThreshold: 1.01\nminMargin: 0.0030000000000000001\nedgeBlurSize: 5\npass2Only: 0\n";
        String actual = readFile(filename);
        actual = actual.replaceAll("e([+-])0(\\d\\d)", "e$1$2"); // NOTE: workaround for different platforms double representation
        assertEquals(truth, actual);
>>>>>>> dd08328228f008f270a199b7fb25aab37a91135d
    }

}
