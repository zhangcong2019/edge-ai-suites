import time
import unittest

from selenium import webdriver
from selenium.webdriver.common.by import By


class ImageBasedVideoSearchTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.driver = webdriver.Chrome()
        cls.driver.get("http://localhost:3000")
        time.sleep(2)

    @classmethod
    def tearDownClass(cls):
        cls.driver.quit()

    def test_startup(self):
        # Assert page title is correct
        self.assertIn("Image Search", self.driver.title)

        # Assert that map loads
        map_element = self.driver.find_element(By.ID, "map")
        self.assertIsNotNone(map_element)

        # Assert that video loads
        video_iframe = self.driver.find_element(
            By.XPATH, "//iframe[@title='Local Stream']"
        )
        self.driver.switch_to.frame(video_iframe)
        video_element = self.driver.find_element(By.ID, "video")
        self.driver.switch_to.default_content()
        self.assertIsNotNone(video_element)

        # Assert that buttons loads
        button_texts = [
            "Analyze Stream",
            "Capture Frame",
            "Upload Image",
            "Clear Database",
        ]
        for text in button_texts:
            button = self.driver.find_element(
                By.XPATH, f"//button[contains(text(), '{text}')]"
            )
            self.assertIsNotNone(button)

    def test_live_search(self):

        # Select "Analyze Stream" button
        analyze_button = self.driver.find_element(
            By.XPATH, "//button[contains(text(), 'Analyze Stream')]"
        )

        # Start Analysis
        analyze_button.click()

        # Do analysis for 60 seconds
        time.sleep(60)

        # Select "Stop Analysis" button
        stop_button = self.driver.find_element(
            By.XPATH, "//button[contains(text(), 'Stop Analysis')]"
        )

        # Stop Analysis
        stop_button.click()

        # Select "Capture Frame" button
        capture_button = self.driver.find_element(
            By.XPATH, "//button[contains(text(), 'Capture Frame')]"
        )

        # Capture a Frame
        capture_button.click()

        # Wait 2 seconds for capture to complete
        time.sleep(2)

        # Select "Search Object" button
        search_button = self.driver.find_element(
            By.XPATH, "//button[contains(text(), 'Search Object')]"
        )

        # Run a search
        search_button.click()

        # Wait 5 seconds for the search to complete
        time.sleep(5)

        # Assert that there are 10 image results
        image_list = self.driver.find_element(By.CLASS_NAME, "image-list")
        image_items = image_list.find_elements(By.TAG_NAME, "li")
        self.assertEqual(len(image_items), 10)

        # Select "Clear Database" button
        clear_button = self.driver.find_element(
            By.XPATH, "//button[contains(text(), 'Clear Database')]"
        )

        # Clear the database
        clear_button.click()


if __name__ == "__main__":
    suite = unittest.TestSuite()
    suite.addTest(ImageBasedVideoSearchTest("test_startup"))
    suite.addTest(ImageBasedVideoSearchTest("test_live_search"))
    runner = unittest.TextTestRunner()
    runner.run(suite)
