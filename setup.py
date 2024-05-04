from setuptools import setup
from setuptools import Extension

setup(
    name='simple_graphs',
    version='1.0.0',
    author='Krystian Jandy, s184589',
    author_email='s184589@student.pg.edu.pl',
    url='https://github.com/Krystian030/simple-graphs',
    ext_modules=[Extension('simple_graphs', ['simple_graphs.c'])],
)