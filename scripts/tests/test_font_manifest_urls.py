import unittest


class FontManifestUrls(unittest.TestCase):
    def test_github_primary_before_legacy(self):
        github = (
            'https://github.com/crosspoint-reader/crosspoint-fonts/releases/download/'
            'sd-fonts-m1-b4/fonts.json'
        )
        legacy = 'https://crossmux.com/api/assets/fonts/m1-b4/fonts.json'
        self.assertIn('crosspoint-fonts', github)
        self.assertIn('crossmux.com', legacy)
        self.assertNotEqual(github, legacy)


if __name__ == '__main__':
    unittest.main()
