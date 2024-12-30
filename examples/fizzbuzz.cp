let fizz_buzz = fn(x) {
    let i = 0;
    while (i < x) {
        i = i + 1;
        if (i % 15 == 0) {
            puts("FizzBuzz");
            continue;
        }
        if (i % 3 == 0) {
            puts("Fizz");
            continue;
        }
        if (i % 5 == 0) {
            puts("Buzz");
            continue;
        }
        puts(i);
    }
}

fizz_buzz(100);

